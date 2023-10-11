#pragma once

#include <memory>
#include <vector>

namespace apa102 {

struct SetOptions {
  double src = 1.0;
  double dst = 1.0;
};

struct Buffer {
 public:
  explicit Buffer(size_t num_leds);

  void clear();
  void set(size_t i, uint8_t r, uint8_t g, uint8_t b, const SetOptions& options = {});

  size_t numLeds() const;

  uint8_t* data();
  size_t size() const;

 private:
  size_t _num_leds;
  std::vector<uint8_t> _buf;
};

struct LED {
  virtual ~LED() = default;
  virtual void show(Buffer&) = 0;
};

std::unique_ptr<LED> createLED(int hz = 2000000);

}  // namespace apa102
