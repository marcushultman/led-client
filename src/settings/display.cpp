#include "display.h"

#include <iostream>

#include "encoding/base64.h"

extern "C" {
#include <jq.h>
}

namespace settings {

DisplayService::DisplayService(spotiled::BrightnessProvider &brightness)
    : _brightness(brightness) {}

void DisplayService::handleUpdate(std::string_view data) {
  auto jv_dict = jv_parse(encoding::base64::decode(data).c_str());
  if (jv_get_kind(jv_dict) != JV_KIND_OBJECT) {
    jv_free(jv_dict);
    return;
  }
  auto jv_brightness = jv_object_get(jv_copy(jv_dict), jv_string("brightness"));
  auto jv_hue = jv_object_get(jv_copy(jv_dict), jv_string("hue"));

  if (jv_get_kind(jv_brightness) == JV_KIND_NUMBER) {
    _brightness.setBrightness(int(jv_number_value(jv_brightness)));
  }
  if (jv_get_kind(jv_hue) == JV_KIND_NUMBER) {
    _brightness.setHue(int(jv_number_value(jv_hue)));
  }

  jv_free(jv_hue);
  jv_free(jv_brightness);
  jv_free(jv_dict);

  std::cout << "DisplayService brightness: " << int(_brightness.brightness())
            << " hue: " << int(_brightness.hue()) << std::endl;
}

}  // namespace settings
