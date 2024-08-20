#include "display.h"

namespace web_proxy {
namespace {

//

}  // namespace

void Display::onRenderPass(render::LED &led, std::chrono::milliseconds elapsed) {
  led.setLogo(logo);

  auto w = std::max<int>(width, 23);
  int h = height ? height : 16;
  int t = xscroll ? xscroll * elapsed.count() / 1000 : 0;

  for (auto i = 0; i < bytes.size() / 4; ++i) {
    int x = xscroll ? 23 + (i / h - t) % w : i / h;
    int y = i % h;

    auto *p = reinterpret_cast<uint8_t *>(bytes.data() + 4 * i);
    auto [r, g, b, a] = std::tie(p[0], p[1], p[2], p[3]);

    led.set({x, y}, {r, g, b}, {.src = uint8_t(a) / 255.0});
  }
}

}  // namespace web_proxy
