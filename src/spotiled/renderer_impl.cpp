#include <spotiled/renderer_impl.h>

#include <queue>

namespace spotiled {
namespace {

struct RendererImpl final : public Renderer {
  RendererImpl(async::Scheduler &main_scheduler, std::unique_ptr<BufferedLED> led)
      : _main_scheduler{main_scheduler}, _led{std::move(led)} {}

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

std::unique_ptr<Renderer> createRenderer(async::Scheduler &main_scheduler,
                                         std::unique_ptr<BufferedLED> led) {
  return std::make_unique<RendererImpl>(main_scheduler, std::move(led));
}

}  // namespace spotiled
