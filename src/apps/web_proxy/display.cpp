#include "display.h"

namespace web_proxy {
namespace {

//

}  // namespace

void Display::onRenderPass(spotiled::LED &led, std::chrono::milliseconds elapsed) {
  led.setLogo(logo);

  for (auto i = 0; i < bytes.size() / 4; ++i) {
    auto *p = bytes.data() + 4 * i;
    auto [r, g, b, a] = std::tie(p[0], p[1], p[2], p[3]);
    led.set({i / 16, i % 16}, Color(r, g, b), {.src = uint8_t(a) / 255.0});
  }

  // renderRolling(led, elapsed, page);
}

}  // namespace web_proxy
