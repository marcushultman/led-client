#pragma once

#include <string_view>

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"

struct UIService : present::Presentable {
  virtual ~UIService() = default;
  virtual http::Response handleRequest(http::Request) = 0;
};

std::unique_ptr<UIService> makeUIService(async::Scheduler &,
                                         http::Http &,
                                         present::PresenterQueue &,
                                         std::string_view base_url);
