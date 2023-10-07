#include "display.h"

#include <charconv>
#include <cmath>
#include <fstream>

#include "brightness_provider.h"
#include "time_of_day_brightness.h"
#include "util/color/color.h"
#include "util/spotiled/spotiled.h"
#include "util/url/url.h"

namespace settings {
namespace {

using namespace std::chrono_literals;

constexpr auto kDefaultBrightness = 32;
constexpr auto kDefaultHue = 255;

constexpr auto kFilename = "brightness0";
constexpr auto kTimeout = 3s;

}  // namespace

DisplayService::DisplayService(async::Scheduler &main_scheduler, present::PresenterQueue &presenter)
    : _main_scheduler{main_scheduler},
      _presenter{presenter},
      _brightness(kDefaultBrightness),
      _hue{kDefaultHue} {
  if (auto stream = std::ifstream(kFilename); stream.good()) {
    std::string line(64, '\0');
    stream.getline(line.data(), line.size());
    _brightness = std::stoi(line);
    stream.getline(line.data(), line.size());
    _hue = std::stoi(line);
  }
  printf("DisplayService brightness: %d hue: %d\n", _brightness, _hue);
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
    _brightness = std::stoi(req.body);
  } else if (setting == "hue") {
    _hue = std::stoi(req.body);
  }
  printf("DisplayService brightness: %d hue: %d\n", _brightness, _hue);
  save();

  if (!_notified && !_timeout.count()) {
    _presenter.add(*this, {.prio = present::Prio::kNotification});
  }
  _notified = true;
  return 204;
}

Color DisplayService::logoBrightness() const { return timeOfDayBrightness(_brightness); }

Color DisplayService::brightness() const { return timeOfDayBrightness(3 * _brightness / 4); }

void DisplayService::start(SpotiLED &led, present::Callback callback) {
  led.add([this, callback = std::move(callback)](auto &led, auto elapsed) {
    if (std::exchange(_notified, false)) {
      _timeout = elapsed + kTimeout;
    }
    if (elapsed >= _timeout) {
      stop();
      callback();
      return false;
    }
    led.setLogo(logoBrightness());
    double bump = elapsed.count();
    for (auto x = 0; x < 23; ++x) {
      auto a = 8 + 8 * std::sin(2 * M_PI * (bump / 50 + x) / 20);
      auto b = 8 + 3 * std::sin(2 * M_PI * (bump / 50 + x) / 20);

      for (auto y = std::min<int>(std::floor(a), std::floor(b)),
                end = std::max<int>(std::ceil(a), std::ceil(b));
           y < end; y++) {
        led.set({x, y}, {brightness()[0], 0, 0});
      }
    }

    for (auto x = 0; x < 23; ++x) {
      auto a = 8 + 6 * std::sin(2 * M_PI * (bump / 33 + x) / 25);
      auto b = 8 + 3 * std::sin(2 * M_PI * (bump / 33 + x) / 25);

      for (auto y = std::min<int>(std::floor(a), std::floor(b)),
                end = std::max<int>(std::ceil(a), std::ceil(b));
           y < end; y++) {
        led.blend({x, y}, {0, 0, brightness()[0]});
      }
    }

    for (auto x = 0; x < 23; ++x) {
      auto a = 8 + 4 * std::sin(2 * M_PI * (bump / 25 + x) / 30);
      auto b = 8 + 2 * std::sin(2 * M_PI * (bump / 25 + x) / 30);

      for (auto y = std::min<int>(std::floor(a), std::floor(b)),
                end = std::max<int>(std::ceil(a), std::ceil(b));
           y < end; y++) {
        led.blend({x, y}, {0, brightness()[0], 0});
      }
    }

    return true;
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
