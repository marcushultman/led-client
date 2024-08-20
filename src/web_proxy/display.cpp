#include "display.h"

#include <cmath>

namespace web_proxy {
namespace {

auto roundUp(double d) { return d < 0 ? std::ceil(d) : std::floor(d); }

}  // namespace

void Display::onRenderPass(render::LED &led, std::chrono::milliseconds elapsed) {
  led.setLogo(logo);

  int w = std::max<int>(width, 23);
  int h = height ? height : 16;
  int t = xscroll ? xscroll * elapsed.count() / 1000 : 0;

  for (auto i = 0; i < bytes.size() / 4; ++i) {
    auto sx = i / h;
    int x = xscroll ? 23 + (sx - t) % w : sx;
    int y = i % h;

    if (wave) {
      y += roundUp((7.5 - y) * 0.5 *
                   (1 + std::sin(-M_PI * std::sin(sx * M_PI_2 / 23) +
                                 wave * (elapsed.count() * M_2_PI) / 1000)));
    }

    auto *p = reinterpret_cast<uint8_t *>(bytes.data() + 4 * i);
    auto [r, g, b, a] = std::tie(p[0], p[1], p[2], p[3]);

    led.set({x, y}, {r, g, b}, {.src = uint8_t(a) / 255.0});
  }
}

}  // namespace web_proxy
