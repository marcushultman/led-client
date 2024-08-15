#include "brightness_provider.h"

namespace spotiled {

uint8_t BrightnessProvider::brightness() const { return _brightness; }
uint8_t BrightnessProvider::hue() const { return _hue; }

void BrightnessProvider::setBrightness(uint8_t brightness) { _brightness = brightness; }
void BrightnessProvider::setHue(uint8_t hue) { _hue = hue; }

}  // namespace spotiled