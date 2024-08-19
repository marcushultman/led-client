#include "display.h"

#include <iostream>

#include "encoding/base64.h"

extern "C" {
#include <jq.h>
}

namespace settings {
namespace {

constexpr auto kMinBrightness = 1;
constexpr auto kMaxBrightness = 64 - 1;

constexpr auto kMinHue = 0;
constexpr auto kMaxHue = 255;

}  // namespace

DisplayService::DisplayService(spotiled::BrightnessProvider &brightness)
    : _brightness(brightness) {}

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

  std::cout << "DisplayService brightness" << (on_load ? " (load)" : "") << ": "
            << int(_brightness.brightness()) << " hue: " << int(_brightness.hue()) << std::endl;
}

}  // namespace settings
