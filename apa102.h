#pragma once

#include <memory>
#include <vector>

#if __arm__
#include "spidev_lib++.h"
#endif

namespace apa102 {

class LED {
 public:
  LED(int num_leds, int hz = 4000000) : _num_leds{num_leds} {
#if __arm__
    spi_config_t spi_config;
    spi_config.mode = 0;
    spi_config.speed = hz;
    spi_config.delay = 0;
    spi_config.bits_per_word = 8;
    _spi = std::make_unique<SPI>("/dev/spidev0.0", &spi_config);

    if (!_spi->begin()) {
      printf("SPI failed\n");
    }
#endif
    int trailing = _num_leds / 16;
    if (_num_leds % 16 != 0) {
      ++trailing;
    }
    int capacity = 4 + 4 * _num_leds + trailing;
    _buf.resize(capacity);
    for (auto i = 0; i < trailing; ++i) {
      _buf[(1 + _num_leds) * 4 + i] = 0xff;
    }
    clear();
  }

  void clear() {
    for (auto i = 0; i < _num_leds; ++i) {
      _buf[(1 + i) * 4] = 0xff;
      _buf[(1 + i) * 4 + 1] = 0;
      _buf[(1 + i) * 4 + 2] = 0;
      _buf[(1 + i) * 4 + 3] = 0;
    }
  }

  void set(int i, uint8_t r, uint8_t g, uint8_t b) {
    _buf[(1 + i) * 4 + 1] = b;
    _buf[(1 + i) * 4 + 2] = g;
    _buf[(1 + i) * 4 + 3] = r;
  }

  void show() {
#if __arm__
    for (auto i = 0; i < _buf.size(); i += 0xff) {
      _spi->write(&_buf[i], std::min<int>(0xff, _buf.size() - i));
    }
#endif
  }

#if __arm__
  std::unique_ptr<SPI> _spi;
#endif
  int _num_leds;
  std::vector<uint8_t> _buf;
};

}  // namespace apa102
