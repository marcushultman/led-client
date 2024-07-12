#pragma once

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/brightness_provider.h"
#include "util/spotiled/spotiled.h"

namespace settings {

struct DisplayService : present::Presentable {
  DisplayService(async::Scheduler &, spotiled::BrightnessProvider &, present::PresenterQueue &);
  ~DisplayService();

  http::Response operator()(http::Request);

  void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed) final;
  void onStop() final;

 private:
  void save();

  async::Scheduler &_main_scheduler;
  spotiled::BrightnessProvider &_brightness;
  present::PresenterQueue &_presenter;

  std::chrono::milliseconds _timeout = {};
  bool _notified = false;
};

}  // namespace settings
