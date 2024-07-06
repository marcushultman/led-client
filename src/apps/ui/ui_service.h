#pragma once

#include <string_view>

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct UIService : present::Presentable {
  virtual ~UIService() = default;
  virtual http::Response handleRequest(http::Request) = 0;
};

std::unique_ptr<UIService> makeUIService(async::Scheduler &,
                                         http::Http &,
                                         spotiled::Renderer &,
                                         present::PresenterQueue &,
                                         std::string_view base_url);
