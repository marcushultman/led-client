#pragma once

#include "http/http.h"
#include "present/presenter.h"

struct DrawService final : present::Presentable {
  explicit DrawService(present::PresenterQueue &);

  http::Response handleRequest(http::Request);

  void start(spotiled::Renderer &, present::Callback) final;
  void stop() final;

 private:
  present::PresenterQueue &_presenter;
  std::optional<http::Request> _request;
  std::chrono::system_clock::time_point _expire_at;
};
