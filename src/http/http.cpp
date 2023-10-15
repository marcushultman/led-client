#include "http.h"

#include <curl/curl.h>

#include <array>
#include <atomic>
#include <iostream>
#include <set>

#include "http/util.h"

namespace http {
namespace {

constexpr auto kThreadPoolSize = 4;
constexpr auto kDefaultTimeout = std::chrono::seconds(5);

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

struct RequestState : std::enable_shared_from_this<RequestState> {
  RequestState(Request req, RequestOptions opts) : req{std::move(req)}, opts{std::move(opts)} {}

  void scheduleResponse() {
    _work.insert(opts.post_to.schedule([self = shared_from_this()] { self->onResponse(); }));
  }
  void scheduleCancellation() {
    _work.insert(opts.post_to.schedule([self = shared_from_this()] { self->abort(); }));
  }

  void onResponse() {
    if (auto callback = std::exchange(opts.callback, {})) {
      callback(std::move(response));
    }
  }
  void onTimeout() {
    if (auto callback = std::exchange(opts.callback, {})) {
      callback(Response(408));
    }
  }
  void abort() { opts.callback = {}; }

  Request req;
  RequestOptions opts;
  Response response;

 private:
  std::set<async::Lifetime> _work;
};

class RequestExecutor final {
 public:
  RequestExecutor(CURL *curl, async::Scheduler &scheduler, std::shared_ptr<RequestState> state)
      : _state{state},
        _work{scheduler.schedule([curl, state] { performRequest(curl, std::move(state)); })},
        _timeout{shouldUseTimeout(state->req)
                     ? state->opts.post_to.schedule([state] { state->onTimeout(); },
                                                    {.delay = kDefaultTimeout})
                     : nullptr} {}
  ~RequestExecutor() { _state->scheduleCancellation(); }

  static void performRequest(CURL *curl, std::shared_ptr<RequestState> state) {
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, state->req.url.c_str());

    setMethod(curl, state->req.method);

    curl_slist *headers = nullptr;
    for (auto &[key, val] : state->req.headers) {
      auto new_headers = curl_slist_append(headers, (key + ": " + val).c_str());
      headers = new_headers ? new_headers : (curl_slist_free_all(headers), nullptr);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (!state->req.body.empty()) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, state->req.body.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HEADERDATA, state.get());
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHeader);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, state.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onBytes);

    if (auto failed = curl_easy_perform(curl); !failed) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &state->response.status);
    }
    state->scheduleResponse();
  }

  static size_t onHeader(char *ptr, size_t size, size_t nmemb, void *request) {
    size *= nmemb;
    auto buffer = std::string_view(ptr, size);
    if (buffer.starts_with("HTTP") || buffer.find('\r') == 0) {
      return size;
    }
    auto &res = static_cast<RequestState *>(request)->response;
    auto key = std::string(buffer.substr(0, buffer.find(":")));
    auto value = std::string(trim(buffer.substr(key.size() + 1)));
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    res.headers[std::move(key)] = std::move(value);
    return size;
  }

  static size_t onBytes(char *ptr, size_t size, size_t nmemb, void *request) {
    size *= nmemb;
    static_cast<RequestState *>(request)->response.body.append(ptr, size);
    return size;
  }

 private:
  std::shared_ptr<RequestState> _state;
  async::Lifetime _work;
  async::Lifetime _timeout;
};

class HttpImpl final : public Http {
 public:
  explicit HttpImpl(bool &ok) {
    for (auto &resources : _pool) {
      auto *curl = curl_easy_init();
      resources.curl = curl;
      resources.thread = async::Thread::create("http");
      ok &= bool(curl);
    }
  }
  ~HttpImpl() {
    for (auto &resources : _pool) {
      curl_easy_cleanup(resources.curl);
      resources.thread.reset();
    }
  }

  Lifetime request(Request request, RequestOptions opts) final {
    auto &[curl, thread] = _pool[_next_thread.fetch_add(1) % kThreadPoolSize];
    return std::make_unique<RequestExecutor>(
        curl, thread->scheduler(),
        std::make_shared<RequestState>(std::move(request), std::move(opts)));
  }

 private:
  struct ThreadResources {
    CURL *curl = nullptr;
    std::unique_ptr<async::Thread> thread;
  };
  std::array<ThreadResources, kThreadPoolSize> _pool;
  std::atomic_size_t _next_thread = 0;
};

std::unique_ptr<Http> Http::create() {
  bool ok = true;
  auto http = std::make_unique<HttpImpl>(ok);
  return ok ? std::move(http) : nullptr;
}

}  // namespace http