#include "http.h"

#include <curl/curl.h>

#include <atomic>
#include <iostream>
#include <unordered_map>

#include "http/util.h"

namespace http {
namespace {

constexpr int64_t kMaxBufferSize = 16 * 1024;

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

struct Buffer {
  int64_t offset = 0;
  std::string data;
  size_t skip = 0;
  bool is_processing = false;
};

}  // namespace

Response::Response() = default;

Response::Response(int status) : status{status} {}

Response::Response(std::string body) : status{200}, body{std::move(body)} {}

Response::Response(int status, Headers headers, std::string body)
    : status{status}, headers{std::move(headers)}, body{std::move(body)} {}

struct RequestState {
  RequestState(async::Scheduler &http_scheduler, Request &&request, RequestOptions &&opts)
      : request{std::move(request)}, opts{std::move(opts)}, _http_scheduler{http_scheduler} {}

  Request request;
  RequestOptions opts;
  Response response;
  std::atomic_bool aborted = false;
  std::shared_ptr<Buffer> buffer;

  void runOnHttp(async::Fn fn) { _http_work = _http_scheduler.schedule(std::move(fn)); }
  void runOnMain(async::Fn fn) { _main_work = opts.post_to.schedule(std::move(fn)); }

 private:
  async::Scheduler &_http_scheduler;
  async::Lifetime _main_work, _http_work;
};

class RequestHandle final {
 public:
  RequestHandle(std::shared_ptr<CURLM> curlm, std::shared_ptr<RequestState> request)
      : _curlm{std::move(curlm)}, _request{std::move(request)} {}
  ~RequestHandle() {
    _request->aborted = true;
    if (auto curl = _curlm.lock()) {
      curl_multi_wakeup(curl.get());
    }
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
    auto state =
        std::make_shared<RequestState>(_thread->scheduler(), std::move(request), std::move(opts));

    state->runOnHttp([this, state] { processNewRequest(state); });

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
        finishRequest(info->easy_handle, info->data.result,
                      std::move(_requests[info->easy_handle]));
        _requests.erase(info->easy_handle);
      }
    }

    if (!err) {
      for (auto &[curl, state] : _requests) {
        if (state->buffer) {
          processStream(curl, state, *state->buffer);
        }
      }
      if (still_running) {
        err = curl_multi_poll(_curlm.get(), nullptr, 0, 1000, NULL);
      }
    }

    if (err) {
      std::cerr << "curl failed: " << curl_multi_strerror(err) << std::endl;
      return;
    }

    _work = still_running ? _thread->scheduler().schedule([this] { process(); }) : nullptr;
  }

  void setupRequest(std::shared_ptr<RequestState> state) {
    auto curl = curl_easy_init();

    if (!curl) {
      return state->runOnMain([state] {
        if (!state->aborted) {
          state->opts.on_response(500);
        }
      });
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
    } else {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
    }

    curl_easy_setopt(curl, CURLOPT_HEADERDATA, state.get());
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHeader);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, state.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onBytes);

    curl_multi_add_handle(_curlm.get(), curl);

    _requests[curl] = state;
  }

  void processStream(CURL *curl, std::shared_ptr<RequestState> &state, Buffer &buffer) {
    if (buffer.is_processing || buffer.data.size() < kMaxBufferSize) {
      return;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &state->response.status);
    buffer.is_processing = true;

    state->runOnMain([this, curl, state] {
      triggerCallbacks(*state, /* skip_empty */ false, [this, curl, state] {
        state->runOnHttp([this, curl, state] { continueRequest(curl, state); });
        curl_multi_wakeup(_curlm.get());
      });
    });
  }

  void continueRequest(CURL *curl, std::shared_ptr<RequestState> state) {
    if (state->aborted) {
      return;
    }
    state->buffer->offset += state->buffer->data.size();
    state->buffer->data.clear();
    state->buffer->is_processing = false;
    curl_easy_pause(curl, CURLPAUSE_CONT);
    process();
  }

  void finishRequest(CURL *curl, CURLcode code, std::shared_ptr<RequestState> state) {
    if (code == CURLE_OK) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &state->response.status);
    } else {
      std::cerr << "curl failed: " << curl_easy_strerror(code) << std::endl;
    }
    curl_multi_remove_handle(_curlm.get(), curl);
    curl_easy_cleanup(curl);

    state->runOnMain([state] { triggerCallbacks(*state, /* skip_empty */ true, [state] {}); });
  }

  static void triggerCallbacks(RequestState &state, bool skip_empty, std::function<void()> next) {
    if (state.aborted) {
      return;
    }
    if (auto on_response = std::exchange(state.opts.on_response, {})) {
      on_response(std::move(state.response));
    }
    if (state.aborted) {
      return;
    }
    if (auto &buffer = state.buffer) {
      if (auto on_bytes = state.opts.on_bytes; on_bytes && (!skip_empty || !buffer->data.empty())) {
        struct BufferHandle {
          BufferHandle(std::function<void()> next) : next{std::move(next)} {}
          ~BufferHandle() { next(); }
          std::function<void()> next;
        };
        on_bytes(buffer->offset, buffer->data, std::make_shared<BufferHandle>(std::move(next)));
      } else {
        next();
      }
    }
  }

  static size_t onHeader(char *ptr, size_t size, size_t nmemb, void *obj) {
    auto state = static_cast<RequestState *>(obj);
    size *= nmemb;
    auto buffer = std::string_view(ptr, size);
    if (buffer.starts_with("HTTP") || buffer.find('\r') == 0) {
      return size;
    }
    auto key = std::string(buffer.substr(0, buffer.find(":")));
    auto value = std::string(trim(buffer.substr(key.size() + 1)));
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    if (key == "content-length") {
      if (auto content_length = std::stoll(value);
          content_length < kMaxBufferSize || !state->opts.on_bytes) {
        state->response.body.reserve(content_length);
      } else {
        state->buffer = std::make_unique<Buffer>();
        state->buffer->data.reserve(kMaxBufferSize);
      }
    }

    state->response.headers[std::move(key)] = std::move(value);
    return size;
  }

  static size_t onBytes(char *ptr, size_t size, size_t nmemb, void *obj) {
    auto state = static_cast<RequestState *>(obj);
    size *= nmemb;
    auto chunk = std::string_view(ptr, size);

    if (auto &buffer = state->buffer) {
      if (buffer->data.size() + size > kMaxBufferSize) {
        auto overflow = buffer->data.size() + size - kMaxBufferSize;
        auto partial = chunk.substr(0, chunk.size() - overflow);
        buffer->data += partial;
        buffer->skip = partial.size();
        return CURL_WRITEFUNC_PAUSE;
      }
      buffer->data += chunk.substr(std::exchange(buffer->skip, 0));
    } else {
      state->response.body += chunk;
    }

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