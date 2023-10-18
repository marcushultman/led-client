#pragma once

#include <string>

#include "apps/settings/display.h"
#include "async/scheduler.h"
#include "http/http.h"

namespace web_proxy {

class WebProxy {
 public:
  WebProxy(async::Scheduler &, http::Http &, settings::Settings &, std::string base_url);
  http::Lifetime handleRequest(http::Request, http::RequestOptions::OnResponse);

 private:
  async::Scheduler &_main_scheduler;
  http::Http &_http;
  settings::Settings &_settings;
  std::string _base_url;
};

}  // namespace web_proxy
