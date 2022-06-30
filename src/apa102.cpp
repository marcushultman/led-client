#include "apa102.h"

#include <iostream>
#include <string>

#if __arm__
#include "spidev_lib++.h"
#endif

namespace apa102 {

#if __arm__

class SPILED final : public LED {
 public:
  SPILED(int num_leds, int hz) : LED(num_leds, hz) {
    spi_config_t spi_config;
    spi_config.mode = 0;
    spi_config.speed = hz;
    spi_config.delay = 0;
    spi_config.bits_per_word = 8;
    _spi = std::make_unique<SPI>("/dev/spidev0.0", &spi_config);

    if (!_spi->begin()) {
      printf("SPI failed\n");
      _spi.reset();
    }
  }

  void show() {
    if (_spi) {
      _spi->write(_buf.data(), _buf.size());
    }
  }
  std::unique_ptr<SPI> _spi;
};

#endif

class Simulator final : public LED {
 public:
  Simulator(int num_leds, int hz) : LED(num_leds, hz) {}

  void show() {
    std::string s;
    s.reserve(_num_leds);
    for (auto i = 0; i < _num_leds; ++i) {
      auto b = _buf[(1 + i) * 4 + 1];
      auto g = _buf[(1 + i) * 4 + 2];
      auto r = _buf[(1 + i) * 4 + 3];
      s += (r > std::max(b, g) ? std::string("❤️") : (b > g ? "💙" : "💚"));
    }
    std::cout << s.c_str() << std::endl;
  }
};

std::unique_ptr<LED> createLED(int num_leds, int hz) {
#if __arm__
  return std::make_unique<SPILED>(num_leds, hz);
#else
  return std::make_unique<Simulator>(num_leds, hz);
#endif
}

}  // namespace apa102
