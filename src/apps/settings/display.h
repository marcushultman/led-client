#pragma once

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
#include "spotiled/brightness_provider.h"
#include "spotiled/spotiled.h"

namespace settings {

struct DisplayService : present::Presentable {
  DisplayService(async::Scheduler &, spotiled::BrightnessProvider &, present::PresenterQueue &);
  ~DisplayService();

  http::Response operator()(http::Request);
  void handleUpdate(std::string_view data, bool on_load);

  void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed) final;
  void onStop() final;

 private:
  void onSettingsUpdated();
  void save();

  async::Scheduler &_main_scheduler;
  spotiled::BrightnessProvider &_brightness;
  present::PresenterQueue &_presenter;

  std::chrono::milliseconds _timeout = {};
  bool _notified = false;
};

}  // namespace settings
