#include "apa102.h"

#if __arm__
#include "spidev_lib++.h"
#else
#include <cassert>
#include <fstream>
#endif

namespace apa102 {

Buffer::Buffer(size_t num_leds) : _num_leds{num_leds} {
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

void Buffer::clear() {
  for (auto i = 0; i < _num_leds; ++i) {
    _buf[4 + 4 * i] = 0xff;
    _buf[4 + 4 * i + 1] = 0;
    _buf[4 + 4 * i + 2] = 0;
    _buf[4 + 4 * i + 3] = 0;
  }
}

void Buffer::set(size_t i, uint8_t r, uint8_t g, uint8_t b) {
  if (i >= _num_leds) {
    return;
  }
  _buf[4 + 4 * i + 1] = b;
  _buf[4 + 4 * i + 2] = g;
  _buf[4 + 4 * i + 3] = r;
}

void Buffer::blend(size_t i, uint8_t r, uint8_t g, uint8_t b, float blend) {
  if (i >= _num_leds) {
    return;
  }
  auto &sb = _buf[4 + 4 * i + 1];
  auto &sg = _buf[4 + 4 * i + 2];
  auto &sr = _buf[4 + 4 * i + 3];
  auto inv_blend = 1 - blend;
  sb = inv_blend * sb + blend * b;
  sg = inv_blend * sg + blend * g;
  sr = inv_blend * sr + blend * r;
}

size_t Buffer::numLeds() const { return _num_leds; }

uint8_t *Buffer::data() { return _buf.data(); }
size_t Buffer::size() const { return _buf.size(); }

#if __arm__

class SPILED final : public LED {
 public:
  explicit SPILED(int hz)
      : _config{
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

  void show(Buffer &buffer) final {
    if (_spi) {
      _spi->write(buffer.data(), buffer.size());
    }
  }

 private:
  spi_config_t _config;
  std::unique_ptr<SPI> _spi;
};

#else

std::string_view simLogo(const uint8_t *abgr) {
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

std::string_view simBrightness(const uint8_t *abgr) {
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

class Simulator final : public LED {
 public:
  Simulator() : _pipe("./simulator_out") {}

  void show(Buffer &buffer) final {
    assert(buffer.numLeds() == 19 + 16 * 23);
    auto *data = buffer.data();
    _pipe << "\n\n\n\n";
    for (auto i = 0; i < 19; ++i) {
      auto *abgr = &data[(1 + i) * 4];
      _pipe << simLogo(abgr) << simBrightness(abgr);
    }
    _pipe << "\n\n";

    for (auto y = 0; y < 16; ++y) {
      for (auto x = 0; x < 23; ++x) {
        auto i = 19 + x * 16 + 15 - y % 16;
        auto *abgr = &data[(1 + i) * 4];
        _pipe << " " << simLogo(abgr) << simBrightness(abgr);
      }
      _pipe << std::endl;
    }
  }

 private:
  std::ofstream _pipe;
};

#endif

std::unique_ptr<LED> createLED(int hz) {
#if __arm__
  return std::make_unique<SPILED>(hz);
#else
  return std::make_unique<Simulator>();
#endif
}

}  // namespace apa102
