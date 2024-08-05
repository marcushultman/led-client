#pragma once

#include <string>

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
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
           std::string base_url);
  ~WebProxy();
  http::Lifetime handleRequest(http::Request,
                               http::RequestOptions::OnResponse,
                               http::RequestOptions::OnBytes);

 private:
  void requestStateUpdate(std::string id, State &);

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  spotiled::BrightnessProvider &_brightness;
  std::string _base_url;
  std::string_view _base_host;
  std::unique_ptr<StateThingy> _state_thingy;
};

}  // namespace web_proxy
