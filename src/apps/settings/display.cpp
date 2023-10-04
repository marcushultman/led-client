#include "display.h"

#include <charconv>
#include <fstream>

#include "brightness_provider.h"
#include "spotiled.h"
#include "time_of_day_brightness.h"
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

  if (!std::exchange(_pending_present, true)) {
    _presenter.add(*this);
  } else if (_led) {
    update();
  }
  return 204;
}

Color DisplayService::logoBrightness() const { return timeOfDayBrightness(_brightness); }

Color DisplayService::brightness() const { return timeOfDayBrightness(3 * _brightness / 4); }

void DisplayService::start(SpotiLED &led, present::Callback callback) {
  _led = &led;
  _callback = std::move(callback);
  update();
}

void DisplayService::stop() {
  _led->clear();
  _led->show();
  _led = nullptr;
  _lifetime.reset();
  _pending_present = false;
}

void DisplayService::update() {
  _led->clear();
  _led->setLogo(_brightness);
  for (auto i = 0; i < 23; ++i) {
    _led->set({i, 8}, _brightness);
    _led->set({i, 9}, _brightness);
  }
  _led->show();

  _lifetime = _main_scheduler.schedule(
      [this] {
        stop();
        _callback();
      },
      {.delay = kTimeout});
}

void DisplayService::save() {
  auto line = std::to_string(_brightness) + "\n" + std::to_string(_hue) + "\n";
  std::ofstream(kFilename).write(line.data(), line.size());
}

}  // namespace settings
