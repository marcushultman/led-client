#include "display.h"

#include <charconv>
#include <fstream>

#include "brightness_provider.h"
#include "time_of_day_brightness.h"
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
    auto bump = (elapsed.count() / 50) % 23;
    for (auto i = 0; i < 23; ++i) {
      led.set({i, i == bump ? 6 : 7}, brightness());
      led.set({i, i == bump ? 7 : 8}, brightness());
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
