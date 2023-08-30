#include "http.h"

#include <curl/curl.h>

#include <iostream>
#include <set>

namespace http {
namespace {

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

}  // namespace

struct RequestState : std::enable_shared_from_this<RequestState> {
  explicit RequestState(RequestInit init) : init{std::move(init)} {}

  void scheduleResponse() {
    _work.insert(init.post_to.schedule([self = shared_from_this()] { self->onResponse(); }));
  }
  void scheduleCancellation() {
    _work.insert(init.post_to.schedule([self = shared_from_this()] { self->abort(); }));
  }

  void onResponse() {
    if (auto callback = std::exchange(init.callback, {})) {
      callback(std::move(response));
    }
  }
  void onTimeout() {
    if (auto callback = std::exchange(init.callback, {})) {
      callback({.status = 408});
    }
  }
  void abort() { init.callback = {}; }

  RequestInit init;
  Response response;

 private:
  std::set<async::Lifetime> _work;
};

class RequestImpl final : public Request {
 public:
  RequestImpl(CURL *curl, async::Scheduler &scheduler, std::shared_ptr<RequestState> state)
      : _state{state},
        _work{scheduler.schedule([curl, state] { performRequest(curl, std::move(state)); })},
        _timeout{state->init.post_to.schedule([state] { state->onTimeout(); },
                                              {.delay = kDefaultTimeout})} {}
  ~RequestImpl() { _state->scheduleCancellation(); }

  static void performRequest(CURL *curl, std::shared_ptr<RequestState> state) {
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, state->init.url.c_str());

    setMethod(curl, state->init.method);

    curl_slist *headers = nullptr;
    for (auto &[key, val] : state->init.headers) {
      auto new_headers = curl_slist_append(headers, (key + ": " + val).c_str());
      headers = new_headers ? new_headers : (curl_slist_free_all(headers), nullptr);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (!state->init.body.empty()) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, state->init.body.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, state.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onBytes);

    if (auto failed = curl_easy_perform(curl); !failed) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &state->response.status);
    }
    state->scheduleResponse();
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
  explicit HttpImpl(CURL *curl) : _curl{curl} {}
  ~HttpImpl() { curl_easy_cleanup(_curl); }

  std::unique_ptr<Request> request(RequestInit init) final {
    return std::make_unique<RequestImpl>(_curl, _thread->scheduler(),
                                         std::make_shared<RequestState>(std::move(init)));
  }

 private:
  CURL *_curl;
  std::unique_ptr<async::Thread> _thread = async::Thread::create("http");
};

std::unique_ptr<Http> Http::create() {
  if (auto *curl = curl_easy_init()) {
    return std::make_unique<HttpImpl>(curl);
  }
  return nullptr;
}

}  // namespace http