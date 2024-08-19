#include "updater.h"

#include <iostream>

#include "encoding/base64.h"

extern "C" {
#include <jq.h>
}

namespace settings {

void updateSettings(Settings &settings, std::string_view data) {
  auto jv_dict = jv_parse(encoding::base64::decode(data).c_str());
  if (jv_get_kind(jv_dict) != JV_KIND_OBJECT) {
    jv_free(jv_dict);
    return;
  }
  auto jv_brightness = jv_object_get(jv_copy(jv_dict), jv_string("brightness"));
  auto jv_hue = jv_object_get(jv_copy(jv_dict), jv_string("hue"));

  if (jv_get_kind(jv_brightness) == JV_KIND_NUMBER) {
    settings.brightness = jv_number_value(jv_brightness);
  }
  if (jv_get_kind(jv_hue) == JV_KIND_NUMBER) {
    settings.hue = jv_number_value(jv_hue);
  }

  jv_free(jv_hue);
  jv_free(jv_brightness);
  jv_free(jv_dict);

  std::cout << "settings brightness: " << int(settings.brightness) << " hue: " << int(settings.hue)
            << std::endl;
}

}  // namespace settings
