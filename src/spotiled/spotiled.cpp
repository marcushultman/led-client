#include "spotiled/spotiled.h"

#include <apa102/apa102.h>
#include <render/renderer_impl.h>

#include "color/color.h"

namespace spotiled {
namespace {

using namespace render;

struct SpotiLED final : BufferedLED {
  SpotiLED() { _led->show(*_buffer); }

 private:
  void clear() final { _buffer->clear(); }
  void show() final { _led->show(*_buffer); }

  void setLogo(Color color, const Options &options) final {
    auto [r, g, b] = color;
    for (auto i = 0; i < 19; ++i) {
      _buffer->set(i, r, g, b, options);
    }
  }
  void set(Coord pos, Color color, const Options &options) final {
    if (pos.x >= 0 && pos.x < 23 && pos.y >= 0 && pos.y < 16) {
      auto [r, g, b] = color;
      _buffer->set(19 + offset(pos), r, g, b, options);
    }
  }
  size_t offset(Coord pos) { return 16 * pos.x + 15 - pos.y; }

  std::unique_ptr<led::LED> _led = apa102::createLED();
  std::unique_ptr<led::Buffer> _buffer{_led->createBuffer()};
};

}  // namespace

std::unique_ptr<Renderer> create(async::Scheduler &main_scheduler) {
  return createRenderer(main_scheduler, std::make_unique<SpotiLED>());
}

}  // namespace spotiled