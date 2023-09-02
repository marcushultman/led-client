#include "apa102.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#if __arm__
#include "spidev_lib++.h"
#endif

namespace apa102 {

class LEDBase : public LED {
 public:
  LEDBase(size_t num_leds, size_t hz) : _num_leds{num_leds} {
    auto trailing = _num_leds / 16;
    if (_num_leds % 16 != 0) {
      ++trailing;
    }
    auto capacity = 4 + 4 * _num_leds + trailing;
    _buf.resize(capacity);
    for (auto i = 0; i < trailing; ++i) {
      _buf[4 + 4 * _num_leds + i] = 0xff;
    }
    clear();
  }

  void clear() {
    for (auto i = 0; i < _num_leds; ++i) {
      _buf[4 + 4 * i] = 0xff;
      _buf[4 + 4 * i + 1] = 0;
      _buf[4 + 4 * i + 2] = 0;
      _buf[4 + 4 * i + 3] = 0;
    }
  }

  void set(size_t i, uint8_t r, uint8_t g, uint8_t b) {
    if (i >= _num_leds) {
      return;
    }
    _buf[4 + 4 * i + 1] = b;
    _buf[4 + 4 * i + 2] = g;
    _buf[4 + 4 * i + 3] = r;
  }

 protected:
  size_t _num_leds;
  std::vector<uint8_t> _buf;
};

#if __arm__

class SPILED final : public LEDBase {
 public:
  SPILED(int num_leds, int hz)
      : LEDBase(num_leds, hz),
        _config{
            .mode = 0,
            .bits_per_word = 8,
            .speed = hz,
            .delay = 0,
        } {
    _spi = std::make_unique<SPI>("/dev/spidev0.0", &_config);

    if (!_spi->begin()) {
      _spi.reset();
    }
  }

  void show() {
    if (_spi) {
      _spi->write(_buf.data(), _buf.size());
    }
  }

 private:
  spi_config_t _config;
  std::unique_ptr<SPI> _spi;
};

#endif

std::string_view simLogo(uint8_t *abgr) {
  auto b = *(abgr + 1), g = *(abgr + 2), r = *(abgr + 3);
  if (r > g) {
    return (r > b ? "🔴" : b > r ? "🔵" : "🟣");
  } else if (g > b) {
    return (g > r ? "🟢" : r > g ? "🔴" : "🟡");
  } else if (b > r) {
    return (b > g ? "🔵" : g > b ? "🟢" : "🩵");
  } else {
    return (r ? "⚪️" : "⚫️");
  }
}

std::string_view simBrightness(uint8_t *abgr) {
  auto b = *(abgr + 1), g = *(abgr + 2), r = *(abgr + 3);
  if ((r + g + b) == 0) {
    return " ";
  } else if ((r + g + b) < 255) {
    return ".";
  } else if ((r + g + b) < (255 * 2)) {
    return "-";
  } else {
    return "|";
  }
}

class Simulator final : public LEDBase {
 public:
  Simulator(int num_leds, int hz) : LEDBase(num_leds, hz), _pipe("./simulator_out") {}

  void show() {
    assert(_num_leds == 19 + 16 * 23);
    _pipe << "\n\n\n\n";
    for (auto i = 0; i < 19; ++i) {
      auto *abgr = &_buf[(1 + i) * 4];
      _pipe << simLogo(abgr) << simBrightness(abgr);
    }
    _pipe << "\n\n";

    for (auto y = 0; y < 16; ++y) {
      for (auto x = 0; x < 23; ++x) {
        auto i = 19 + x * 16 + 15 - y % 16;
        auto *abgr = &_buf[(1 + i) * 4];
        _pipe << " " << simLogo(abgr) << simBrightness(abgr);
      }
      _pipe << std::endl;
    }
  }

 private:
  std::ofstream _pipe;
};

std::unique_ptr<LED> createLED(int num_leds, int hz) {
#if __arm__
  return std::make_unique<SPILED>(num_leds, hz);
#else
  return std::make_unique<Simulator>(num_leds, hz);
#endif
}

}  // namespace apa102
