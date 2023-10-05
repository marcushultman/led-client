#pragma once

#include <memory>

namespace apa102 {

struct LED {
  virtual ~LED() = default;

  virtual void clear() = 0;
  virtual void set(size_t i, uint8_t r, uint8_t g, uint8_t b) = 0;
  virtual void blend(size_t i, uint8_t r, uint8_t g, uint8_t b, float blend = .5f) = 0;
  virtual void show() = 0;
};

std::unique_ptr<LED> createLED(int num_leds, int hz = 2000000);

}  // namespace apa102
