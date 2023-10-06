#pragma once

#include <memory>
#include <vector>

namespace apa102 {

struct Buffer {
 public:
  explicit Buffer(size_t num_leds);

  void clear();
  void set(size_t i, uint8_t r, uint8_t g, uint8_t b);
  void blend(size_t i, uint8_t r, uint8_t g, uint8_t b, float blend = .5f);

  size_t numLeds() const;

  const uint8_t* data() const;
  size_t size() const;

 private:
  size_t _num_leds;
  std::vector<uint8_t> _buf;
};

struct LED {
  virtual ~LED() = default;
  virtual void show(const Buffer&) = 0;
};

std::unique_ptr<LED> createLED(int hz = 2000000);

}  // namespace apa102
