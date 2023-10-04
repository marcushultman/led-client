#pragma once

#include "util/color/color.h"

namespace settings {

struct BrightnessProvider {
  virtual ~BrightnessProvider() = default;
  virtual Color logoBrightness() const = 0;
  virtual Color brightness() const = 0;
};

}  // namespace settings
