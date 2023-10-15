#include "web_proxy.h"

#include "util/url/url.h"

namespace web_proxy {
namespace {

const auto kDefaultBaseUrl = "https://spotiled.deno.dev";
const auto kHostHeader = "host";

}  // namespace

WebProxy::WebProxy(async::Scheduler &main_scheduler, http::Http &http, std::string base_url)
    : _main_scheduler{main_scheduler},
      _http{http},
      _base_url{base_url.empty() ? kDefaultBaseUrl : std::move(base_url)} {}

http::Lifetime WebProxy::handleRequest(http::Request req,
                                       http::RequestOptions::OnResponse callback) {
  auto suffix = std::string(url::Url(req.url).host.end(), std::string_view(req.url).end());
  req.url = _base_url + suffix;
  if (auto it = req.headers.find(kHostHeader); it != req.headers.end()) {
    it->second = url::Url(req.url).host;
  }
  return _http.request(std::move(req),
                       {.post_to = _main_scheduler, .callback = std::move(callback)});
}

}  // namespace web_proxy
