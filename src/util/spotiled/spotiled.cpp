#include "util/spotiled/spotiled.h"

#include <queue>

#include "util/apa102/apa102.h"

namespace {

struct SpotiLEDImpl final : public SpotiLED {
  SpotiLEDImpl(async::Scheduler &main_scheduler) : _main_scheduler{main_scheduler} { show(); }

  void clear() final { _buffer.clear(); }
  void setLogo(Color color) final {
    auto [r, g, b] = color;
    for (auto i = 0; i < 19; ++i) {
      _buffer.set(i, r, g, b);
    }
  }
  void set(Coord pos, Color color) final {
    if (pos.x >= 0 && pos.x < 23 && pos.y >= 0 && pos.y < 16) {
      auto [r, g, b] = color;
      _buffer.set(19 + offset(pos), r, g, b);
    }
  }
  void blend(Coord pos, Color color, float blend) final {
    if (pos.x >= 0 && pos.x < 23 && pos.y >= 0 && pos.y < 16) {
      auto [r, g, b] = color;
      _buffer.blend(19 + offset(pos), r, g, b, blend);
    }
  }
  void show() final { _led->show(_buffer); }

  void add(RenderCallback callback) final {
    _pending_callbacks.push(std::move(callback));
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

    clear();
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
    show();

    if (!_callbacks.empty()) {
      _render = _main_scheduler.schedule([this] { renderFrame(); }, {.delay = delay});
    }
  }

  size_t offset(Coord pos) { return 16 * pos.x + 15 - pos.y; }

  async::Scheduler &_main_scheduler;
  apa102::Buffer _buffer{19 + 16 * 23};
  std::unique_ptr<apa102::LED> _led = apa102::createLED();
  std::vector<std::pair<RenderCallback, std::chrono::system_clock::time_point>> _callbacks;
  std::queue<RenderCallback> _pending_callbacks;
  async::Lifetime _render;
};

}  // namespace

std::unique_ptr<SpotiLED> SpotiLED::create(async::Scheduler &main_scheduler) {
  return std::make_unique<SpotiLEDImpl>(main_scheduler);
}