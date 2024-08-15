#include "spotiled/spotiled.h"

#include <apa102/apa102.h>

#include <queue>

#include "color/color.h"
#include "time_of_day_brightness.h"

namespace spotiled {
namespace {

struct BufferedLED : LED {
  virtual void clear() = 0;
  virtual void show() = 0;
};

struct SpotiLED final : BufferedLED {
  explicit SpotiLED(BrightnessProvider &brightness) : _brightness{brightness} {
    _led->show(*_buffer);
  }

 private:
  void clear() final { _buffer->clear(); }
  void show() final { _led->show(*_buffer); }

  void setLogo(Color color, const Options &options) final {
    auto [r, g, b] = color * timeOfDayBrightness(_brightness);
    for (auto i = 0; i < 19; ++i) {
      _buffer->set(i, r, g, b, options);
    }
  }
  void set(Coord pos, Color color, const Options &options) final {
    if (pos.x >= 0 && pos.x < 23 && pos.y >= 0 && pos.y < 16) {
      auto [r, g, b] = color * timeOfDayBrightness(_brightness);
      _buffer->set(19 + offset(pos), r, g, b, options);
    }
  }
  size_t offset(Coord pos) { return 16 * pos.x + 15 - pos.y; }

  BrightnessProvider &_brightness;
  std::unique_ptr<led::LED> _led = apa102::createLED();
  std::unique_ptr<led::Buffer> _buffer{_led->createBuffer()};
};

struct RendererImpl final : public Renderer {
  RendererImpl(async::Scheduler &main_scheduler, BrightnessProvider &brightness)
      : _main_scheduler{main_scheduler}, _led{std::make_unique<SpotiLED>(brightness)} {}

  void add(RenderCallback callback) final {
    _pending_callbacks.push(std::move(callback));
    notify();
  }

  void notify() final {
    _render = _main_scheduler.schedule([this] { renderFrame(); });
  }

 private:
  void renderFrame() {
    using namespace std::chrono_literals;
    auto now = std::chrono::system_clock::now();

    while (!_pending_callbacks.empty()) {
      _callbacks.emplace_back(std::move(_pending_callbacks.front()), now);
      _pending_callbacks.pop();
    }

    auto delay = std::chrono::milliseconds(1min);

    _led->clear();
    for (auto it = _callbacks.begin(); it != _callbacks.end();) {
      auto &callback = it->first;
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);

      if (auto next_frame = callback(*_led, elapsed); next_frame.count()) {
        delay = std::min(next_frame, delay);
        ++it;
      } else {
        it = _callbacks.erase(it);
      }
    }
    _led->show();

    if (!_callbacks.empty()) {
      _render = _main_scheduler.schedule([this] { renderFrame(); }, {.delay = delay});
    }
  }

  async::Scheduler &_main_scheduler;
  std::unique_ptr<BufferedLED> _led;
  std::vector<std::pair<RenderCallback, std::chrono::system_clock::time_point>> _callbacks;
  std::queue<RenderCallback> _pending_callbacks;
  async::Lifetime _render;
};

}  // namespace

uint8_t BrightnessProvider::brightness() const { return _brightness; }
uint8_t BrightnessProvider::hue() const { return _hue; }

void BrightnessProvider::setBrightness(uint8_t brightness) { _brightness = brightness; }
void BrightnessProvider::setHue(uint8_t hue) { _hue = hue; }

std::unique_ptr<Renderer> create(async::Scheduler &main_scheduler, BrightnessProvider &brightness) {
  return std::make_unique<RendererImpl>(main_scheduler, brightness);
}

}  // namespace spotiled