#pragma once

#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct DrawService final : present::Presentable {
  DrawService(async::Scheduler &main_scheduler, present::PresenterQueue &);

  http::Response handleRequest(http::Request);

  void onStart() final;
  void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed) final;
  void onStop() final;

 private:
  async::Scheduler &_main_scheduler;
  present::PresenterQueue &_presenter;
  std::optional<http::Request> _request;
  async::Lifetime _expire;
};
