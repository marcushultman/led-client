#include "util/spotiled/spotiled.h"

#include "util/apa102/apa102.h"

std::unique_ptr<SpotiLED> SpotiLED::create() {
  struct SpotiLEDImpl final : public SpotiLED {
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

   private:
    size_t offset(Coord pos) { return 16 * pos.x + 15 - pos.y; }

    apa102::Buffer _buffer{19 + 16 * 23};
    std::unique_ptr<apa102::LED> _led = apa102::createLED();
  };
  return std::make_unique<SpotiLEDImpl>();
}
