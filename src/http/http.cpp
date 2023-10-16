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

struct RequestExecutor : std::enable_shared_from_this<RequestExecutor> {
  RequestExecutor(Request req, RequestOptions opts)
      : _mcurl{curl_multi_init()}, _req{std::move(req)}, _opts{std::move(opts)} {}
  ~RequestExecutor() { curl_multi_cleanup(_mcurl); }

  void performRequest(CURL *curl) {
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, _req.url.c_str());

    setMethod(curl, _req.method);

    curl_slist *headers = nullptr;
    for (auto &[key, val] : _req.headers) {
      auto new_headers = curl_slist_append(headers, (key + ": " + val).c_str());
      headers = new_headers ? new_headers : (curl_slist_free_all(headers), nullptr);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (!_req.body.empty()) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, _req.body.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &_response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHeader);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &_response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onBytes);

    std::cout << "http " << _req.url << ": curl perform" << std::endl;
    curl_multi_add_handle(_mcurl, curl);

    auto err = CURLM_OK;
    auto still_running = 0;
    do {
      if (err = curl_multi_perform(_mcurl, &still_running); !err && still_running) {
        err = curl_multi_poll(_mcurl, nullptr, 0, 1000, nullptr);
      }
    } while (!err && still_running && !_aborted);

    if (err) {
      std::cout << "http " << _req.url << ": ERRROR: " << err << std::endl;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &_response.status);
    scheduleResponse();

    std::cout << "http " << _req.url << ": " << (_aborted ? "aborted" : "done") << std::endl;
    curl_multi_remove_handle(_mcurl, curl);
  }

  void abort() {
    _aborted = true;
    curl_multi_wakeup(_mcurl);
  }

 private:
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

  void scheduleResponse() {
    _work = _opts.post_to.schedule([this, self = shared_from_this()] {
      if (!_aborted) {
        _opts.callback(std::move(_response));
      }
    });
  }

  CURLM *_mcurl = nullptr;
  Request _req;
  RequestOptions _opts;
  Response _response;
  std::atomic_bool _aborted = false;
  async::Lifetime _work;
};

class RequestHandle final {
 public:
  RequestHandle(CURL *curl, async::Scheduler &scheduler, std::shared_ptr<RequestExecutor> exec)
      : _exec{exec}, _work{scheduler.schedule([curl, exec] { exec->performRequest(curl); })} {}
  ~RequestHandle() { _exec->abort(); }

 private:
  std::shared_ptr<RequestExecutor> _exec;
  async::Lifetime _work;
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
    return std::make_unique<RequestHandle>(
        curl, thread->scheduler(),
        std::make_shared<RequestExecutor>(std::move(request), std::move(opts)));
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