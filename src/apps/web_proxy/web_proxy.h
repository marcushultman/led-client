#pragma once

#include "async/scheduler.h"
#include "http/http.h"

namespace web_proxy {

class WebProxy {
 public:
  WebProxy(async::Scheduler &, http::Http &);
  http::Lifetime handleRequest(http::Request, http::RequestOptions::OnResponse);

 private:
  async::Scheduler &_main_scheduler;
  http::Http &_http;
};

}  // namespace web_proxy
