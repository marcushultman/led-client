#pragma once

#include <charconv>

#include "font/font.h"
#include "service/service_runner.h"
#include "spotiled.h"

constexpr auto kDelayHeader = "delay";

struct TextPrompt : service::API {
  TextPrompt(async::Scheduler &main_scheduler,
             SpotiLED &led,
             BrightnessProvider &brightness_provider)
      : _main_scheduler{main_scheduler}, _led{led}, _brightness_provider{brightness_provider} {}
  ~TextPrompt() {
    _led.clear();
    _led.show();
  }

  std::chrono::milliseconds timeout(const http::Request &req) {
    if (auto it = req.headers.find(kDelayHeader); it != req.headers.end()) {
      int delay_s = 0;
      auto value = std::string_view(it->second);
      std::from_chars(value.begin(), value.end(), delay_s);
      return std::chrono::seconds(delay_s);
    }
    return {};
  }

  http::Response handleRequest(http::Request req) {
    if (req.method != http::Method::POST) {
      return 400;
    }
    auto text = std::string(req.body.substr(req.body.find_first_of("=") + 1));
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);
    _page->setText(std::move(text));

    _runner = RollingPresenter::create(_main_scheduler, _led, _brightness_provider, *_page,
                                       Direction::kHorizontal, {});
    return 204;
  }

 private:
  async::Scheduler &_main_scheduler;
  SpotiLED &_led;
  BrightnessProvider &_brightness_provider;
  async::Lifetime _runner;
  std::unique_ptr<font::TextPage> _page = font::TextPage::create();
};
