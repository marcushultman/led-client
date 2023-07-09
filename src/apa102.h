#pragma once

#include <algorithm>
#include <memory>
#include <vector>

namespace apa102 {

class LED {
 public:
  LED(int num_leds, int hz) : _num_leds{num_leds} {
    int trailing = _num_leds / 16;
    if (_num_leds % 16 != 0) {
      ++trailing;
    }
    int capacity = 4 + 4 * _num_leds + trailing;
    _buf.resize(capacity);
    std::fill(_buf.begin(), _buf.end(), 0);
    for (auto i = 0; i < trailing; ++i) {
      _buf[4 + 4 * _num_leds + i] = 0xff;
    }
    clear();
  }
  virtual ~LED() = default;

  void clear() {
    for (auto i = 0; i < _num_leds; ++i) {
      _buf[4 + 4 * i] = 0xff;
      _buf[4 + 4 * i + 1] = 0;
      _buf[4 + 4 * i + 2] = 0;
      _buf[4 + 4 * i + 3] = 0;
    }
  }

  void set(int i, uint8_t r, uint8_t g, uint8_t b) {
    if (i < 0 || i >= _num_leds) {
      return;
    }
    _buf[4 + 4 * i + 1] = b;
    _buf[4 + 4 * i + 2] = g;
    _buf[4 + 4 * i + 3] = r;
  }

  virtual void show() = 0;

 protected:
  int _num_leds;
  std::vector<uint8_t> _buf;
};

std::unique_ptr<LED> createLED(int num_leds, int hz = 6000000);

}  // namespace apa102
