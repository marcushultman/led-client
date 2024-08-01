#include "display.h"

namespace web_proxy {
namespace {

//

}  // namespace

void Display::onRenderPass(spotiled::LED &led, std::chrono::milliseconds elapsed) {
  led.setLogo(logo);

  auto w = width ? width : 23;
  auto h = height ? height : 16;
  auto offset = xscroll ? int(xscroll * elapsed.count() / 1000 % w) : 0;

  for (auto i = 0; i < bytes.size() / 4; ++i) {
    auto x = int(i / h);
    auto y = int(i % h);

    x += -offset + ((x < offset) * w);

    auto *p = reinterpret_cast<uint8_t *>(bytes.data() + 4 * i);
    auto [r, g, b, a] = std::tie(p[0], p[1], p[2], p[3]);

    led.set({x, y}, {r, g, b}, {.src = uint8_t(a) / 255.0});
  }

  // renderRolling(led, elapsed, page);
}

}  // namespace web_proxy
