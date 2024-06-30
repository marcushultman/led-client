#pragma once

#include "http/http.h"
#include "present/presenter.h"

struct FlagService : present::Presentable {
  FlagService(present::PresenterQueue &presenter);

  http::Response handleRequest(http::Request);

  void start(spotiled::Renderer &, present::Callback callback) final;
  void stop() final;

 private:
  present::PresenterQueue &_presenter;
  std::chrono::system_clock::time_point _expire_at;
};
