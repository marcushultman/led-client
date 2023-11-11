#pragma once

#include <charconv>
#include <queue>

#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"

struct FlagService : present::Presentable {
  FlagService(async::Scheduler &main_scheduler, present::PresenterQueue &presenter);

  http::Response handleRequest(http::Request);

  void start(spotiled::Renderer &, present::Callback callback) final;
  void stop() final;

 private:
  async::Scheduler &_main_scheduler;
  present::PresenterQueue &_presenter;
  std::shared_ptr<void> _sentinel;
};
