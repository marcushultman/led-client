#include "web_proxy.h"

#include "encoding/base64.h"
#include "uri/uri.h"

extern "C" {
#include <jq.h>
}

namespace web_proxy {
namespace {

constexpr auto kHostHeader = "host";
constexpr auto kDefaultBaseUrl = "https://spotiled.deno.dev";

}  // namespace

WebProxy::WebProxy(async::Scheduler &main_scheduler,
                   http::Http &http,
                   present::Presenter &presenter,
                   std::string base_url,
                   std::string_view device_id,
                   StateThingy::Callbacks callbacks)
    : _main_scheduler{main_scheduler},
      _http{http},
      _base_url{base_url.empty() ? kDefaultBaseUrl : std::move(base_url)},
      _base_host{uri::Uri(_base_url).authority.host},
      _device_id{device_id},
      _state_thingy{std::make_unique<StateThingy>(
          _main_scheduler,
          [this](auto id, auto &state) { requestStateUpdate(std::move(id), state); },
          presenter,
          std::move(callbacks))} {}

WebProxy::~WebProxy() = default;

WebProxy::RequestHandler WebProxy::asRequestHandler() {
  return [this](auto req, auto opts) { return handleRequest(std::move(req), std::move(opts)); };
}

void WebProxy::updateState(std::string id) { _state_thingy->updateState(id); }

http::Lifetime WebProxy::handleRequest(http::Request req, http::RequestOptions opts) {
  if (req.url.empty() || req.url[0] == '*') {
    req.url = _base_url;
  } else if (req.url[0] == '/') {
    req.url = _base_url + req.url;
  }

  if (auto res = _state_thingy->handleRequest(req)) {
    return opts.post_to.schedule(
        [res = std::move(res), opts = std::move(opts)] { opts.on_response(std::move(*res)); });
  }

  if (auto it = req.headers.find(kHostHeader); it != req.headers.end()) {
    it->second = _base_host;
  }

  auto states = jv_object();
  for (auto &[id, state] : _state_thingy->states()) {
    states = jv_object_set(states, jv_string(id.c_str()), jv_string(state.data.c_str()));
  }

  auto jv = jv_object();
  jv = jv_object_set(jv, jv_string("states"), states);
  jv = jv_dump_string(jv, 0);
  req.headers["x-spotiled"] = encoding::base64::encode(jv_string_value(jv));
  jv_free(jv);

  req.headers["x-device-id"] = _device_id;

  return _http.request(std::move(req), std::move(opts));
}

void WebProxy::requestStateUpdate(std::string id, State &state) {
  auto url = _base_url + (id.starts_with('/') ? "" : "/") + std::string(id);
  state.work = _http.request(
      {.method = http::Method::POST,
       .url = std::move(url),
       .headers = {{"content-type", "application/json"}, {"accept-encoding", "identity"}},
       .body = state.data},
      {.post_to = _main_scheduler, .on_response = [this, id = std::move(id), &state](auto res) {
         _state_thingy->onServiceResponse(std::move(res), std::move(id), state);
       }});
}

}  // namespace web_proxy
