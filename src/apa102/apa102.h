#pragma once

#include <led/led.h>

namespace apa102 {

std::unique_ptr<led::LED> createLED(int hz = 2000000);

}  // namespace apa102
