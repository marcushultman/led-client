#pragma once

#include <string_view>

#include "spotiled/brightness_provider.h"

namespace settings {

struct DisplayService {
  explicit DisplayService(spotiled::BrightnessProvider &);

  void handleUpdate(std::string_view data, bool on_load);

 private:
  spotiled::BrightnessProvider &_brightness;
};

}  // namespace settings
