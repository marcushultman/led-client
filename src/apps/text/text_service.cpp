#include "text_service.h"

#include <charconv>
#include <queue>

#include "font/font.h"
#include "present/rolling_presenter.h"
#include "util/spotiled/spotiled.h"

namespace {

constexpr auto kDelayHeader = "delay";
constexpr auto kDefaultTimeout = std::chrono::seconds{5};
constexpr auto kSpeedHeader = "speed";

std::chrono::milliseconds timeout(const http::Request &req) {
  if (auto it = req.headers.find(kDelayHeader); it != req.headers.end()) {
    int delay_s = 0;
    auto value = std::string_view(it->second);
    std::from_chars(value.begin(), value.end(), delay_s);
    return std::chrono::seconds(delay_s);
  }
  return kDefaultTimeout;
}

double speed(const http::Request &req) {
  if (auto it = req.headers.find(kSpeedHeader); it != req.headers.end()) {
    return std::stod(it->second);
  }
  return 0.005;
}

}  // namespace

TextService::TextService(async::Scheduler &main_scheduler,
                         SpotiLED &led,
                         present::PresenterQueue &presenter,
                         settings::BrightnessProvider &brightness_provider)
    : _main_scheduler{main_scheduler},
      _led{led},
      _presenter{presenter},
      _brightness_provider{brightness_provider} {}

http::Response TextService::handleRequest(http::Request req) {
  if (req.method != http::Method::POST) {
    return 400;
  }
  _requests.push(std::move(req));
  _presenter.add(*this, {.prio = present::Prio::kNotification});

  return 204;
}

void TextService::start(SpotiLED &led, present::Callback callback) {
  if (_requests.empty()) {
    return;
  }
  auto req = std::move(_requests.front());
  _requests.pop();

  auto text = std::string(req.body.substr(req.body.find_first_of("=") + 1));
  std::transform(text.begin(), text.end(), text.begin(), ::toupper);
  _text->setText(std::move(text));

  _sentinel = std::make_shared<bool>(true);
  led.add([this, timeout = timeout(req), speed = speed(req), callback = std::move(callback),
           alive = std::weak_ptr<void>(_sentinel)](auto &led, auto elapsed) {
    using namespace std::chrono_literals;
    if (elapsed >= timeout || alive.expired()) {
      callback();
      return 0ms;
    }
    renderRolling(led, _brightness_provider, elapsed, *_text, {}, kNormalScale, speed);
    return std::min(timeout - elapsed, 100ms);
  });
}

void TextService::stop() {
  _sentinel.reset();
  _presenter.notify();
}
