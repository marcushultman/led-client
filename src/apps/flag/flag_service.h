#pragma once

#include "http/http.h"
#include "present/presenter.h"

struct FlagService : present::Presentable {
  FlagService(present::PresenterQueue &presenter);

  http::Response handleRequest(http::Request);

  void onStart(spotiled::Renderer &) final;
  void onStop() final;

 private:
  present::PresenterQueue &_presenter;
  std::chrono::system_clock::time_point _expire_at;
};
