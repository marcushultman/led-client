#include "http.h"

#include <curl/curl.h>

#include <atomic>
#include <iostream>
#include <unordered_map>

#include "http/util.h"

namespace http {
namespace {

void setMethod(CURL *curl, Method method) {
  switch (method) {
    default:
      break;
    case Method::POST:
      curl_easy_setopt(curl, CURLOPT_POST, 1);
      break;
    case Method::PUT:
      curl_easy_setopt(curl, CURLOPT_PUT, 1);
      break;
    case Method::DELETE:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
      break;
    case Method::HEAD:
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
      break;
  }
}

bool shouldUseTimeout(const Request &req) {
  auto it = req.headers.find("connection");
  return it == req.headers.end() || it->second != "keep-alive";
}

}  // namespace

Response::Response() = default;

Response::Response(int status) : status{status} {}

Response::Response(std::string body) : status{200}, body{std::move(body)} {}

struct RequestState {
  RequestState(Request &&request, RequestOptions &&opts)
      : request{std::move(request)}, opts{std::move(opts)} {}

  Request request;
  RequestOptions opts;
  Response response;
  std::atomic_bool aborted = false;

  void retain(async::Lifetime work) { _work = std::move(work); }

 private:
  async::Lifetime _work;
};

class RequestHandle final {
 public:
  RequestHandle(std::shared_ptr<CURLM> curlm, std::shared_ptr<RequestState> request)
      : _curlm{std::move(curlm)}, _request{std::move(request)} {}
  ~RequestHandle() {
    _request->aborted = true;
    if (auto curl = _curlm.lock()) curl_multi_wakeup(curl.get());
  }

 private:
  std::weak_ptr<CURLM> _curlm;
  std::shared_ptr<RequestState> _request;
};

class HttpImpl final : public Http {
 public:
  explicit HttpImpl(CURLM *curlm)
      : _curlm{curlm, [](auto curlm) { curl_multi_cleanup(curlm); }},
        _thread{async::Thread::create("http")} {}
  ~HttpImpl() { _thread.reset(); }

  Lifetime request(Request request, RequestOptions opts) final {
    auto state = std::make_shared<RequestState>(std::move(request), std::move(opts));

    state->retain(_thread->scheduler().schedule(
        [this, state]() mutable { processNewRequest(std::move(state)); }));

    curl_multi_wakeup(_curlm.get());

    return std::make_shared<RequestHandle>(_curlm, state);
  }

 private:
  void processNewRequest(std::shared_ptr<RequestState> state) {
    if (state->aborted) {
      return;
    }
    setupRequest(std::move(state));
    process();
  }

  void process() {
    for (auto it = _requests.begin(); it != _requests.end();) {
      auto &[curl, state] = *it;
      if (state->aborted) {
        curl_multi_remove_handle(_curlm.get(), curl);
        curl_easy_cleanup(curl);
        it = _requests.erase(it);
      } else {
        ++it;
      }
    }

    int still_running = 0;
    auto err = curl_multi_perform(_curlm.get(), &still_running);

    int msgq = 0;
    while (auto info = curl_multi_info_read(_curlm.get(), &msgq)) {
      if (info->msg == CURLMSG_DONE) {
        finishRequest(info->easy_handle, std::move(_requests[info->easy_handle]));
        _requests.erase(info->easy_handle);
      }
    }

    if (!err && still_running) {
      err = curl_multi_poll(_curlm.get(), nullptr, 0, 1000, NULL);
    }

    if (err) {
      std::cerr << "curl failed: " << curl_multi_strerror(err) << std::endl;
      return;
    }

    if (still_running) {
      _work = _thread->scheduler().schedule([this] { process(); });
    }
  }

  void setupRequest(std::shared_ptr<RequestState> state) {
    auto curl = curl_easy_init();

    if (!curl) {
      return state->retain(state->opts.post_to.schedule([state] {
        if (!state->aborted) {
          state->opts.callback(500);
        }
      }));
    }
    curl_easy_setopt(curl, CURLOPT_URL, state->request.url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    setMethod(curl, state->request.method);

    curl_slist *headers = nullptr;
    for (auto &[key, val] : state->request.headers) {
      auto new_headers = curl_slist_append(headers, (key + ": " + val).c_str());
      headers = new_headers ? new_headers : (curl_slist_free_all(headers), nullptr);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (!state->request.body.empty()) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, state->request.body.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &state->response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHeader);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state->response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onBytes);

    curl_multi_add_handle(_curlm.get(), curl);

    _requests[curl] = state;
  }

  void finishRequest(CURL *curl, std::shared_ptr<RequestState> state) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &state->response.status);
    curl_multi_remove_handle(_curlm.get(), curl);
    curl_easy_cleanup(curl);

    state->retain(state->opts.post_to.schedule([state] {
      if (!state->aborted) {
        state->opts.callback(std::move(state->response));
      }
    }));
  }

  static size_t onHeader(char *ptr, size_t size, size_t nmemb, void *response) {
    size *= nmemb;
    auto buffer = std::string_view(ptr, size);
    if (buffer.starts_with("HTTP") || buffer.find('\r') == 0) {
      return size;
    }
    auto key = std::string(buffer.substr(0, buffer.find(":")));
    auto value = std::string(trim(buffer.substr(key.size() + 1)));
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    static_cast<Response *>(response)->headers[std::move(key)] = std::move(value);
    return size;
  }

  static size_t onBytes(char *ptr, size_t size, size_t nmemb, void *response) {
    size *= nmemb;
    static_cast<Response *>(response)->body.append(ptr, size);
    return size;
  }

  std::shared_ptr<CURLM> _curlm;
  std::unique_ptr<async::Thread> _thread;
  std::unordered_map<CURL *, std::shared_ptr<RequestState>> _requests;
  async::Lifetime _work;
};

std::unique_ptr<Http> Http::create() {
  auto curlm = curl_multi_init();
  return curlm ? std::make_unique<HttpImpl>(curlm) : nullptr;
}

}  // namespace http