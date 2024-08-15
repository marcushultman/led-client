#include "apa102.h"

#if __arm__
#include "spidev_lib++.h"
#else
#include <fstream>
#endif

#include <algorithm>
#include <vector>

namespace apa102 {
namespace {

inline uint8_t luminance(int r, int g, int b) {
  auto p = (299 * r) + (587 * g) + (114 * b);
  return p ? std::max(p / 1000, 1) : 0;
}

}  // namespace

using namespace led;

struct BufferImpl final : Buffer {
  explicit BufferImpl(size_t num_leds);

  void clear() final;
  void set(size_t i, uint8_t r, uint8_t g, uint8_t b, const SetOptions &options = {}) final;

  const uint8_t *data() const final;
  size_t size() const final;

 private:
  size_t _num_leds;
  std::vector<uint8_t> _buf;
};

BufferImpl::BufferImpl(size_t num_leds) : _num_leds{num_leds} {
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

void BufferImpl::clear() {
  for (auto i = 0; i < _num_leds; ++i) {
    _buf[4 + 4 * i] = 0xff;
    _buf[4 + 4 * i + 1] = 0;
    _buf[4 + 4 * i + 2] = 0;
    _buf[4 + 4 * i + 3] = 0;
  }
}

void BufferImpl::set(size_t i, uint8_t r, uint8_t g, uint8_t b, const SetOptions &options) {
  if (i >= _num_leds) {
    return;
  }
  auto &dst_b = _buf[4 + 4 * i + 1];
  auto &dst_g = _buf[4 + 4 * i + 2];
  auto &dst_r = _buf[4 + 4 * i + 3];

  auto sum_b = options.dst * dst_b + options.src * b;
  auto sum_g = options.dst * dst_g + options.src * g;
  auto sum_r = options.dst * dst_r + options.src * r;

  auto max_l = std::max(luminance(dst_r, dst_g, dst_b), luminance(r, g, b));
  auto sum_l = luminance(sum_r, sum_g, sum_b);

  if (sum_l == 0) {
    return;
  }
  auto lum_ratio = float(max_l) / sum_l;
  dst_b = sum_b * lum_ratio;
  dst_g = sum_g * lum_ratio;
  dst_r = sum_r * lum_ratio;
}

const uint8_t *BufferImpl::data() const { return _buf.data(); }
size_t BufferImpl::size() const { return _buf.size(); }

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

  std::unique_ptr<Buffer> createBuffer() final {
    return std::make_unique<BufferImpl>(19 + 16 * 23);
  }

  void show(const Buffer &buffer) final {
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
    return (b > g ? "🔵" : g > b ? "🟢" : "🌐");
  } else {
    return (r ? "⚪️" : "⚫️");
  }
}

std::string simBrightness(const uint8_t *abgr) {
  static auto kHex = "0123456789ABCDEF";
  int b = *(abgr + 1), g = *(abgr + 2), r = *(abgr + 3);
  auto m = std::max({r, g, b});
  return {kHex[m >> 4], kHex[m & 0x0F]};
}

class Simulator final : public LED {
 public:
  Simulator() : _pipe("./simulator_out") {}

  std::unique_ptr<Buffer> createBuffer() final {
    return std::make_unique<BufferImpl>(19 + 16 * 23);
  }

  void show(const Buffer &buffer) final {
    auto *data = buffer.data();
    _pipe << "\n\n\n\n\n\n";

    std::vector<std::pair<int, int> > logo = {
        // clang-format off
        {4, 5},
        {3, 5},
        {0, 10},
        {1, 13},
        {3, 15},
        {4, 15},
        {5, 15},
        {7, 13},
        {8, 8},
        {8, 7},
        {7, 2},
        {5, 0},
        {4, 0},
        {3, 0},
        {0, 3},
        {1, 8},
        {2, 8},
        {5, 8},
        {6, 8},
        // clang-format on
    };

    for (auto y = 0; y < 16; ++y) {
      for (auto x = 0; x < 9; ++x) {
        if (auto it = std::find(logo.begin(), logo.end(), std::pair{x, y}); it != logo.end()) {
          auto i = std::distance(logo.begin(), it);
          auto *abgr = &data[(1 + i) * 4];
          _pipe << simLogo(abgr) << simBrightness(abgr);
        } else {
          _pipe << "    ";
        }
      }
      _pipe << " ";
      for (auto x = 0; x < 23; ++x) {
        auto i = 19 + x * 16 + 15 - y % 16;
        auto *abgr = &data[(1 + i) * 4];
        _pipe << simLogo(abgr) << simBrightness(abgr);
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
