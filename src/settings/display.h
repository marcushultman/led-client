#pragma once

#include "present/presenter.h"
#include "spotiled/brightness_provider.h"
#include "spotiled/spotiled.h"

namespace settings {

struct DisplayService : present::Presentable {
  DisplayService(spotiled::BrightnessProvider &, present::Presenter &);

  void handleUpdate(std::string_view data, bool on_load);

  void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed) final;
  void onStop() final;

 private:
  void onSettingsUpdated();

  spotiled::BrightnessProvider &_brightness;
  present::Presenter &_presenter;

  std::chrono::milliseconds _timeout = {};
  bool _notified = false;
};

}  // namespace settings
