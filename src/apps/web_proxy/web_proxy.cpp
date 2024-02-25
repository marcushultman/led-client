#include "web_proxy.h"

#include "util/encoding/base64.h"
#include "util/url/url.h"

extern "C" {
#include <jq.h>
}

namespace web_proxy {
namespace {

const auto kDefaultBaseUrl = "https://spotiled.deno.dev";
const auto kHostHeader = "host";

std::string makeJSON(const char *key, jv value) {
  auto jv = jv_dump_string(jv_object_set(jv_object(), jv_string(key), value), 0);
  std::string ret = jv_string_value(jv);
  jv_free(jv);
  return ret;
}

}  // namespace

WebProxy::WebProxy(async::Scheduler &main_scheduler,
                   http::Http &http,
                   spotiled::BrightnessProvider &brightness,
                   spotify::SpotifyService &spotify,
                   std::string base_url)
    : _main_scheduler{main_scheduler},
      _http{http},
      _brightness{brightness},
      _spotify{spotify},
      _base_url{base_url.empty() ? kDefaultBaseUrl : std::move(base_url)} {}

http::Lifetime WebProxy::handleRequest(http::Request req,
                                       http::RequestOptions::OnResponse on_response,
                                       http::RequestOptions::OnBytes on_bytes) {
  auto suffix = std::string(url::Url(req.url).host.end(), std::string_view(req.url).end());
  req.url = _base_url + suffix;
  if (auto it = req.headers.find(kHostHeader); it != req.headers.end()) {
    it->second = url::Url(req.url).host;
  }

  req.headers["x-spotiled-brightness"] = std::to_string(_brightness.brightness());
  req.headers["x-spotiled-hue"] = std::to_string(_brightness.hue());
  req.headers["x-spotify-auth"] = _spotify.isAuthenticating() ? "true" : "false";

  auto sp = jv_object();
  sp = jv_object_set(sp, jv_string("isAuthenticating"),
                     _spotify.isAuthenticating() ? jv_true() : jv_false());
  sp = jv_object_set(sp, jv_string("tokens"), _spotify.getTokens());

  auto jv = jv_object();
  jv = jv_object_set(jv, jv_string("brightness"), jv_number(_brightness.brightness()));
  jv = jv_object_set(jv, jv_string("hue"), jv_number(_brightness.hue()));
  jv = jv_object_set(jv, jv_string("spotify"), sp);
  jv = jv_dump_string(jv, 0);
  req.headers["x-spotiled"] = encoding::base64::encode(jv_string_value(jv));
  jv_free(jv);

  return _http.request(std::move(req), {.post_to = _main_scheduler,
                                        .on_response = std::move(on_response),
                                        .on_bytes = std::move(on_bytes)});
}

}  // namespace web_proxy
