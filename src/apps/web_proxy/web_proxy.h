#pragma once

#include <string>

#include "apps/spotify/service.h"
#include "async/scheduler.h"
#include "http/http.h"
#include "util/spotiled/brightness_provider.h"

namespace web_proxy {

class WebProxy {
 public:
  WebProxy(async::Scheduler &,
           http::Http &,
           spotiled::BrightnessProvider &,
           spotify::SpotifyService &,
           std::string base_url);
  http::Lifetime handleRequest(http::Request, http::RequestOptions::OnResponse);

 private:
  async::Scheduler &_main_scheduler;
  http::Http &_http;
  spotiled::BrightnessProvider &_brightness;
  spotify::SpotifyService &_spotify;
  std::string _base_url;
};

}  // namespace web_proxy
