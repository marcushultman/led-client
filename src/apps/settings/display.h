#pragma once

#include "async/scheduler.h"
#include "brightness_provider.h"
#include "http/http.h"
#include "present/presenter.h"

namespace settings {

struct DisplayService : present::Presentable, BrightnessProvider {
  DisplayService(async::Scheduler &, present::PresenterQueue &);
  ~DisplayService();

  http::Response operator()(http::Request);

  Color logoBrightness() const final;
  Color brightness() const final;

  void start(SpotiLED &, present::Callback) final;
  void stop() final;

 private:
  void update();
  void save();

  async::Scheduler &_main_scheduler;
  present::PresenterQueue &_presenter;
  uint8_t _brightness = 0;
  uint8_t _hue = 0;

  bool _pending_present = false;

  SpotiLED *_led = nullptr;
  present::Callback _callback;
  async::Lifetime _lifetime;
};

}  // namespace settings
