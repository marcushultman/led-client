#include <async/scheduler.h>
#include <color/color.h>
#include <ikea/ikea.h>
#include <spotiled/renderer_impl.h>
#include <spotiled/time_of_day_brightness.h>

#include <atomic>

#if !WITH_SIMULATOR
#include "pigpio.h"
#include "spidev_lib++.h"
#else
#include <fstream>
#endif

namespace ikea {
namespace {

using namespace spotiled;

constexpr size_t kWidth = 16;
constexpr size_t kHeight = 16;

struct IkeaLED final : BufferedLED {
  explicit IkeaLED(BrightnessProvider &brightness) : _brightness{brightness} {
    static_assert((kWidth * kHeight) % 4 == 0);
    _data[0].resize(kWidth * kHeight / 8);
    _data[1].resize(kWidth * kHeight / 8);

#if !WITH_SIMULATOR
    _spi = std::make_unique<SPI>("/dev/spidev0.0");
    auto err = gpioInitialise() < 0;

    if (!_spi->begin() || err) {
      _spi.reset();
    }

    gpioSetMode(22, PI_OUTPUT);
#else
    _pipe = decltype(_pipe){"./simulator_out2"};
#endif
    _render_work = _render_thread->scheduler().schedule([this] { render(); });
  }
  ~IkeaLED() {
#if !WITH_SIMULATOR
    gpioTerminate();
#endif
  }

 private:
  void clear() final {
    auto &data = _data[_buf];
    std::fill(begin(data), end(data), 0);
  }
  void show() final { _buf = !_buf; }

  void setLogo(Color color, const Options &options) final {}
  void set(Coord pos, Color color, const Options &options) final {
    if (pos.x >= 0 && pos.x < 16 && pos.y >= 0 && pos.y < 16) {
      auto &data = _data[_buf];
      auto [r, g, b] = color * timeOfDayBrightness(_brightness);
      auto i = offset(pos);
      if (r || g || b) {
        data[i >> 3] |= bitMask(i);
      } else {
        data[i >> 3] &= ~bitMask(i);
      }
    }
  }

  // 20
  // 31
  // 46
  // 57
  size_t offset(Coord pos) {
    auto sec = pos.y >> 2;
    auto x = pos.x >> 3;  // 0-1
    auto y = pos.y & 3;   // 0-4
    auto lower = y >= 2;
    auto index = 2 + lower * 2 + (lower * 4 * x - x * 2) + (y % 2);
    return 64 * sec + 8 * index + (pos.x & 7);
  }
  uint8_t bitMask(size_t i) { return 1 << (7 - (i & 7)); }

  void render() {
    using namespace std::chrono_literals;
    auto &data = _data[!_buf];

#if !WITH_SIMULATOR
    if (_spi) {
      gpioWrite(22, 0);
      _spi->write(data.data(), data.size());
      gpioWrite(22, 1);
    }
    auto delay = 100us;
#else
    _pipe << "\n\n\n\n\n\n";

    for (auto y = 0; y < 16; ++y) {
      for (auto x = 0; x < 16; ++x) {
        auto i = offset({x, y});
        auto val = data[i >> 3] & bitMask(i);
        _pipe << (val ? "⚪️" : "⚫️") << " ";
      }
      _pipe << std::endl;
    }
    auto delay = 100ms;
#endif

    _render_work = _render_thread->scheduler().schedule([this] { render(); }, {.delay = delay});
  }

  BrightnessProvider &_brightness;
  std::array<std::vector<uint8_t>, 2> _data;
  std::atomic_size_t _buf = 0;
  std::unique_ptr<async::Thread> _render_thread = async::Thread::create("ikea");
  async::Lifetime _render_work;

#if !WITH_SIMULATOR
  std::unique_ptr<SPI> _spi;
#else
  std::ofstream _pipe;
#endif
};

}  // namespace

std::unique_ptr<Renderer> create(async::Scheduler &main_scheduler, BrightnessProvider &brightness) {
  return createRenderer(main_scheduler, std::make_unique<IkeaLED>(brightness));
}

}  // namespace ikea
