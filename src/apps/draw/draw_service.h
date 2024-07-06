#pragma once

#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct DrawService final : present::Presentable {
  DrawService(spotiled::Renderer &, present::PresenterQueue &);

  http::Response handleRequest(http::Request);

  void onStart() final;
  void onStop() final;

 private:
  spotiled::Renderer &_renderer;
  present::PresenterQueue &_presenter;
  std::optional<http::Request> _request;
  std::chrono::system_clock::time_point _expire_at;
};
