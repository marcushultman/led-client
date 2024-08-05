#pragma once

#include <cstdint>

namespace spotiled {

class BrightnessProvider final {
 public:
  uint8_t brightness() const;
  uint8_t hue() const;

  void setBrightness(uint8_t);
  void setHue(uint8_t);

 private:
  uint8_t _brightness = 0;
  uint8_t _hue = 0;
};

}  // namespace spotiled
