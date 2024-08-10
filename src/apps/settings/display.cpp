#include "display.h"

#include <cmath>
#include <fstream>
#include <iostream>

#include "color/color.h"
#include "encoding/base64.h"
#include "spotiled/spotiled.h"
#include "uri/uri.h"

extern "C" {
#include <jq.h>
}

namespace settings {
namespace {

using namespace std::chrono_literals;

constexpr auto kMinBrightness = 1;
constexpr auto kMaxBrightness = 64 - 1;

constexpr auto kMinHue = 0;
constexpr auto kMaxHue = 255;

constexpr auto kFilename = "brightness0";
constexpr auto kTimeout = 3s;

std::pair<int, int> waveIndices(
    double speed, uint8_t percent, int x, int width, int top, int bottom) {
  auto x_percent = 100 * double(x) / 23;
  auto f = x_percent < percent ? (percent - x_percent) / double(percent) : 0;
  auto w = f * std::sin(2 * M_PI * (0.002 * speed + x) / width);
  auto y1 = 8 + top * w;
  auto y2 = 8 + bottom * w;
  return {std::min<int>(std::floor(y1), std::floor(y2)),
          std::max<int>(std::ceil(y1), std::ceil(y2))};
}

}  // namespace

DisplayService::DisplayService(async::Scheduler &main_scheduler,
                               spotiled::BrightnessProvider &brightness,
                               present::PresenterQueue &presenter)
    : _main_scheduler{main_scheduler}, _presenter{presenter}, _brightness(brightness) {
  if (auto stream = std::ifstream(kFilename); stream.good()) {
    std::string line(64, '\0');
    stream.getline(line.data(), line.size());
    _brightness.setBrightness(std::stoi(line));
    stream.getline(line.data(), line.size());
    _brightness.setHue(std::stoi(line));
  } else {
    _brightness.setBrightness(kMaxBrightness);
    _brightness.setHue(kMaxHue);
  }
  std::cout << "DisplayService brightness: " << int(_brightness.brightness())
            << " hue: " << int(_brightness.hue()) << std::endl;
}

DisplayService::~DisplayService() { save(); }

http::Response DisplayService::operator()(http::Request req) {
  auto setting = uri::Uri::Path(req.url).back();

  if (req.method == http::Method::GET) {
    if (setting == "brightness") {
      return std::to_string(_brightness.brightness());
    } else if (setting == "hue") {
      return std::to_string(_brightness.hue());
    }
  }
  if (req.method != http::Method::POST || req.body.empty()) {
    return 400;
  }

  if (setting == "brightness") {
    _brightness.setBrightness(std::clamp(std::stoi(req.body), kMinBrightness, kMaxBrightness));
  } else if (setting == "hue") {
    _brightness.setHue(std::clamp(std::stoi(req.body), kMinHue, kMaxHue));
  }
  onSettingsUpdated();
  return 204;
}

void DisplayService::handleUpdate(std::string_view data, bool on_load) {
  auto jv_dict = jv_parse(encoding::base64::decode(data).c_str());
  if (jv_get_kind(jv_dict) != JV_KIND_OBJECT) {
    jv_free(jv_dict);
    return;
  }
  auto jv_brightness = jv_object_get(jv_copy(jv_dict), jv_string("brightness"));
  auto jv_hue = jv_object_get(jv_copy(jv_dict), jv_string("hue"));

  if (jv_get_kind(jv_brightness) == JV_KIND_NUMBER) {
    _brightness.setBrightness(
        std::clamp(int(jv_number_value(jv_brightness)), kMinBrightness, kMaxBrightness));
  }
  if (jv_get_kind(jv_hue) == JV_KIND_NUMBER) {
    _brightness.setHue(std::clamp(int(jv_number_value(jv_hue)), kMinHue, kMaxHue));
  }

  jv_free(jv_hue);
  jv_free(jv_brightness);
  jv_free(jv_dict);

  if (!on_load) {
    onSettingsUpdated();
  } else {
    std::cout << "DisplayService brightness (load): " << int(_brightness.brightness())
              << " hue: " << int(_brightness.hue()) << std::endl;
  }
}

void DisplayService::onSettingsUpdated() {
  std::cout << "DisplayService brightness: " << int(_brightness.brightness())
            << " hue: " << int(_brightness.hue()) << std::endl;
  save();

  if (!_notified && !_timeout.count()) {
    _presenter.add(*this, {.prio = present::Prio::kNotification, .render_period = 1000ms / 15});
  }
  _notified = true;
}

void DisplayService::onRenderPass(spotiled::LED &led, std::chrono::milliseconds elapsed) {
  if (std::exchange(_notified, false)) {
    _timeout = elapsed + kTimeout;
  }
  if (elapsed >= _timeout) {
    onStop();
    _presenter.erase(*this);
    return;
  }
  led.setLogo(color::kWhite);

  auto percent = 100 * _brightness.brightness() / kMaxBrightness;

  for (auto x = 0; x < 23; ++x) {
    for (auto [y, end] = waveIndices(10 * elapsed.count(), percent, x, 20, 8, 3); y < end; y++) {
      led.set({x, y}, Color(0, 128, 255));
    }
    for (auto [y, end] = waveIndices(15 * elapsed.count(), percent, x, 25, 6, 3); y < end; y++) {
      led.set({x, y}, Color(128, 0, 255));
    }
    for (auto [y, end] = waveIndices(20 * elapsed.count(), percent, x, 30, 4, 2); y < end; y++) {
      led.set({x, y}, color::kWhite);
    }
  }
}

void DisplayService::onStop() {
  _timeout = {};
  _notified = false;
}

void DisplayService::save() {
  auto line =
      std::to_string(_brightness.brightness()) + "\n" + std::to_string(_brightness.hue()) + "\n";
  std::ofstream(kFilename).write(line.data(), line.size());
}

}  // namespace settings
