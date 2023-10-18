#include "display.h"

#include <charconv>
#include <cmath>
#include <fstream>
#include <iostream>

#include "brightness_provider.h"
#include "time_of_day_brightness.h"
#include "util/color/color.h"
#include "util/spotiled/spotiled.h"
#include "util/url/url.h"

namespace settings {
namespace {

using namespace std::chrono_literals;

constexpr auto kMinBrightness = 1;
constexpr auto kMaxBrightness = 64 - 1;

constexpr auto kDefaultHue = 255;

constexpr auto kFilename = "brightness0";
constexpr auto kTimeout = 3s;

std::pair<int, int> waveIndices(
    double speed, uint8_t brightness, int x, int width, int top, int bottom) {
  auto x_brightness = kMaxBrightness * double(x) / 23;
  auto f = x_brightness < brightness ? (brightness - x_brightness) / double(brightness) : 0;
  auto w = f * std::sin(2 * M_PI * (0.002 * speed + x) / width);
  auto y1 = 8 + top * w;
  auto y2 = 8 + bottom * w;
  return {std::min<int>(std::floor(y1), std::floor(y2)),
          std::max<int>(std::ceil(y1), std::ceil(y2))};
}

}  // namespace

DisplayService::DisplayService(async::Scheduler &main_scheduler, present::PresenterQueue &presenter)
    : _main_scheduler{main_scheduler},
      _presenter{presenter},
      _brightness(kMaxBrightness),
      _hue{kDefaultHue} {
  if (auto stream = std::ifstream(kFilename); stream.good()) {
    std::string line(64, '\0');
    stream.getline(line.data(), line.size());
    _brightness = std::stoi(line);
    stream.getline(line.data(), line.size());
    _hue = std::stoi(line);
  }
  std::cout << "DisplayService brightness: " << _brightness << " hue: " << _hue << std::endl;
}

DisplayService::~DisplayService() { save(); }

http::Response DisplayService::operator()(http::Request req) {
  auto url = url::Url(req.url);
  auto &setting = url.path.back();

  if (req.method == http::Method::GET) {
    if (setting == "brightness") {
      return std::to_string(_brightness);
    } else if (setting == "hue") {
      return std::to_string(_hue);
    }
  }
  if (req.method != http::Method::POST || req.body.empty()) {
    return 400;
  }

  if (setting == "brightness") {
    _brightness = std::clamp(std::stoi(req.body), kMinBrightness, kMaxBrightness);
  } else if (setting == "hue") {
    _hue = std::clamp(std::stoi(req.body), kMinBrightness, kMaxBrightness);
  }
  std::cout << "DisplayService brightness: " << _brightness << " hue: " << _hue << std::endl;
  save();

  if (!_notified && !_timeout.count()) {
    _presenter.add(*this, {.prio = present::Prio::kNotification});
  }
  _notified = true;
  return 204;
}

uint8_t DisplayService::raw_brightness() const { return _brightness; }

uint8_t DisplayService::hue() const { return _hue; }

Color DisplayService::logoBrightness() const { return timeOfDayBrightness(_brightness); }

Color DisplayService::brightness() const {
  return timeOfDayBrightness(std::max(kMinBrightness, 3 * _brightness / 4));
}

void DisplayService::start(spotiled::Renderer &renderer, present::Callback callback) {
  renderer.add([this, callback = std::move(callback)](auto &led, auto elapsed) {
    using namespace std::chrono_literals;

    if (std::exchange(_notified, false)) {
      _timeout = elapsed + kTimeout;
    }
    if (elapsed >= _timeout) {
      stop();
      callback();
      return 0ms;
    }
    led.setLogo(logoBrightness());

    for (auto x = 0; x < 23; ++x) {
      for (auto [y, end] = waveIndices(10 * elapsed.count(), _brightness, x, 20, 8, 3); y < end;
           y++) {
        led.set({x, y}, Color(0, 128, 255) * brightness());
      }
      for (auto [y, end] = waveIndices(15 * elapsed.count(), _brightness, x, 25, 6, 3); y < end;
           y++) {
        led.set({x, y}, Color(128, 0, 255) * brightness());
      }
      for (auto [y, end] = waveIndices(20 * elapsed.count(), _brightness, x, 30, 4, 2); y < end;
           y++) {
        led.set({x, y}, brightness());
      }
    }

    return 1000ms / 15;
  });
}

void DisplayService::stop() {
  _timeout = {};
  _notified = false;
}

void DisplayService::save() {
  auto line = std::to_string(_brightness) + "\n" + std::to_string(_hue) + "\n";
  std::ofstream(kFilename).write(line.data(), line.size());
}

}  // namespace settings
