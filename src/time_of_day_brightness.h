#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>

inline uint8_t brightnessForTimeOfDay(int hour) {
  return 32 + (255 - 32) * std::sin(M_PI * ((24 + hour - 3) % 24) / 24);
}

inline int getHour() {
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  return std::localtime(&now)->tm_hour;
}

inline uint8_t timeOfDayBrightness(uint8_t b, int hour = getHour()) {
  return (brightnessForTimeOfDay(hour) * b) / 255;
}
