#pragma once

#include <string>

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
#include "state_thingy.h"

namespace web_proxy {

class WebProxy {
 public:
  using RequestHandler = std::function<http::Lifetime(http::Request, http::RequestOptions)>;

  WebProxy(async::Scheduler &,
           http::Http &,
           present::Presenter &,
           std::string base_url,
           std::string_view device_id,
           StateThingy::Callbacks);
  ~WebProxy();

  RequestHandler asRequestHandler();

 private:
  http::Lifetime handleRequest(http::Request, http::RequestOptions);
  void requestStateUpdate(std::string id, State &);

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  std::string _base_url;
  std::string_view _base_host;
  std::string_view _device_id;
  std::unique_ptr<StateThingy> _state_thingy;
};

}  // namespace web_proxy
