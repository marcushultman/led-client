#include "util/spotiled/spotiled.h"

#include <iostream>
#include <queue>

#include "time_of_day_brightness.h"
#include "util/color/color.h"

namespace spotiled {
namespace {

const auto kWarm = std::array<Color, 6>{{{155, 41, 72},
                                         {255, 114, 81},
                                         {255, 202, 123},
                                         {255, 205, 116},
                                         {255, 237, 191},
                                         {255, 255, 255}}};

Color hueFactor(double zero_to_one) {
  auto s = zero_to_one * (kWarm.size() - 1);
  auto i = int(s);
  return i == kWarm.size() - 1 ? kWarm[i] : kWarm[i] + (s - i) * (kWarm[i + 1] - kWarm[i]);
}

struct RendererImpl final : public Renderer, LED {
  RendererImpl(async::Scheduler &main_scheduler, BrightnessProvider &brightness)
      : _main_scheduler{main_scheduler}, _brightness{brightness} {
    _led->show(_buffer);
  }

  void add(RenderCallback callback) final {
    _pending_callbacks.push(std::move(callback));
    notify();
  }

  void notify() {
    _render = _main_scheduler.schedule([this] { renderFrame(); });
  }

 private:
  void setLogo(Color color, const Options &options) final {
    auto [r, g, b] = color * timeOfDayBrightness(_brightness.brightness()) *
                     hueFactor(_brightness.hue() / 255.0);
    for (auto i = 0; i < 19; ++i) {
      _buffer.set(i, r, g, b, options);
    }
  }
  void set(Coord pos, Color color, const Options &options) final {
    if (pos.x >= 0 && pos.x < 23 && pos.y >= 0 && pos.y < 16) {
      auto [r, g, b] = color * timeOfDayBrightness(std::max(1, 3 * _brightness.brightness() / 4)) *
                       hueFactor(_brightness.hue() / 255.0);
      _buffer.set(19 + offset(pos), r, g, b, options);
    }
  }

  void renderFrame() {
    using namespace std::chrono_literals;
    auto now = std::chrono::system_clock::now();

    while (!_pending_callbacks.empty()) {
      _callbacks.emplace_back(std::move(_pending_callbacks.front()), now);
      _pending_callbacks.pop();
    }

    auto delay = std::chrono::milliseconds(1min);

    _buffer.clear();
    for (auto it = _callbacks.begin(); it != _callbacks.end();) {
      auto &callback = it->first;
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);

      if (auto next_frame = callback(*this, elapsed); next_frame.count()) {
        delay = std::min(next_frame, delay);
        ++it;
      } else {
        it = _callbacks.erase(it);
      }
    }
    _led->show(_buffer);

    if (!_callbacks.empty()) {
      _render = _main_scheduler.schedule([this] { renderFrame(); }, {.delay = delay});
    }
  }

  size_t offset(Coord pos) { return 16 * pos.x + 15 - pos.y; }

  async::Scheduler &_main_scheduler;
  BrightnessProvider &_brightness;
  apa102::Buffer _buffer{19 + 16 * 23};
  std::unique_ptr<apa102::LED> _led = apa102::createLED();
  std::vector<std::pair<RenderCallback, std::chrono::system_clock::time_point>> _callbacks;
  std::queue<RenderCallback> _pending_callbacks;
  async::Lifetime _render;
};

}  // namespace

uint8_t BrightnessProvider::brightness() const { return _brightness; }
uint8_t BrightnessProvider::hue() const { return _hue; }

void BrightnessProvider::setBrightness(uint8_t brightness) { _brightness = brightness; }
void BrightnessProvider::setHue(uint8_t hue) { _hue = hue; }

std::unique_ptr<Renderer> Renderer::create(async::Scheduler &main_scheduler,
                                           BrightnessProvider &brightness) {
  return std::make_unique<RendererImpl>(main_scheduler, brightness);
}

}  // namespace spotiled