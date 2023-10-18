#pragma once

#include <charconv>
#include <queue>

#include "apps/settings/brightness_provider.h"
#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"

struct TextService : present::Presentable {
  TextService(async::Scheduler &main_scheduler,
              present::PresenterQueue &presenter,
              settings::BrightnessProvider &brightness_provider);

  http::Response handleRequest(http::Request req);

  void start(spotiled::Renderer &, present::Callback callback) final;
  void stop() final;

 private:
  async::Scheduler &_main_scheduler;
  present::PresenterQueue &_presenter;
  settings::BrightnessProvider &_brightness_provider;
  std::unique_ptr<font::TextPage> _text = font::TextPage::create();
  std::queue<http::Request> _requests;
  std::shared_ptr<void> _sentinel;
};
