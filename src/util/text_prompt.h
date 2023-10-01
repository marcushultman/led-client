#pragma once

#include <charconv>
#include <queue>

#include "font/font.h"
#include "http/http.h"
#include "spotiled.h"
#include "util/presenter.h"

struct TextService : presenter::Presentable {
  TextService(async::Scheduler &main_scheduler,
              SpotiLED &led,
              presenter::PresenterQueue &presenter,
              BrightnessProvider &brightness_provider);

  http::Response handleRequest(http::Request req);

  void start(SpotiLED &, presenter::Callback callback) final;
  void stop() final;

 private:
  async::Scheduler &_main_scheduler;
  SpotiLED &_led;
  presenter::PresenterQueue &_presenter;
  BrightnessProvider &_brightness_provider;
  std::unique_ptr<font::TextPage> _page = font::TextPage::create();
  std::queue<http::Request> _requests;

  async::Lifetime _lifetime;
};
