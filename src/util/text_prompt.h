#pragma once

#include <charconv>
#include <queue>

#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"
#include "spotiled.h"

struct TextService : present::Presentable {
  TextService(async::Scheduler &main_scheduler,
              SpotiLED &led,
              present::PresenterQueue &presenter,
              BrightnessProvider &brightness_provider);

  http::Response handleRequest(http::Request req);

  void start(SpotiLED &, present::Callback callback) final;
  void stop() final;

 private:
  async::Scheduler &_main_scheduler;
  SpotiLED &_led;
  present::PresenterQueue &_presenter;
  BrightnessProvider &_brightness_provider;
  std::unique_ptr<font::TextPage> _page = font::TextPage::create();
  std::queue<http::Request> _requests;

  async::Lifetime _lifetime;
};
