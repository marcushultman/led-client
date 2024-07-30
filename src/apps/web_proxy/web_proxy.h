#pragma once

#include <string>

#include "apps/spotify/service.h"
#include "async/scheduler.h"
#include "http/http.h"
#include "util/spotiled/brightness_provider.h"

namespace web_proxy {

struct State;
struct StateThingy;

class WebProxy {
 public:
  WebProxy(async::Scheduler &,
           http::Http &,
           spotiled::BrightnessProvider &,
           present::PresenterQueue &,
           spotify::SpotifyService &,
           std::string base_url);
  ~WebProxy();
  http::Lifetime handleRequest(http::Request,
                               http::RequestOptions::OnResponse,
                               http::RequestOptions::OnBytes);

 private:
  void requestStateUpdate(std::string id, State &, bool retry_allowed);

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  spotiled::BrightnessProvider &_brightness;
  spotify::SpotifyService &_spotify;
  std::string _base_url;
  std::string_view _base_host;
  std::unique_ptr<StateThingy> _state_thingy;
};

}  // namespace web_proxy
