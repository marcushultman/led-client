#pragma once

#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct FlagService : present::Presentable {
  FlagService(spotiled::Renderer &, present::PresenterQueue &);

  http::Response handleRequest(http::Request);

  void onStart() final;
  void onStop() final;

 private:
  spotiled::Renderer &_renderer;
  present::PresenterQueue &_presenter;
  std::chrono::system_clock::time_point _expire_at;
};
