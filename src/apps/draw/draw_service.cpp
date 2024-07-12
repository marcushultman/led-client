#include "apps/draw/draw_service.h"

#include <iostream>

namespace {

using namespace std::chrono_literals;

}  // namespace

DrawService::DrawService(async::Scheduler &main_scheduler, present::PresenterQueue &presenter)
    : _main_scheduler{main_scheduler}, _presenter{presenter} {}

http::Response DrawService::handleRequest(http::Request req) {
  if (req.method != http::Method::POST || req.body.size() != 16 * 23 * 3) {
    return 400;
  }

  auto render_period = 100ms;

  if (auto it = _request->headers.find("x-draw-fps"); it != _request->headers.end()) {
    render_period = std::chrono::milliseconds(1000 / std::stoi(it->second));
  }

  if (!std::exchange(_request, req)) {
    std::cout << "draw started" << std::endl;
    _presenter.add(*this, {.prio = present::Prio::kNotification, .render_period = render_period});

  } else {
    std::cout << "draw updated" << std::endl;
    _presenter.notify();
  }
  _expire = _main_scheduler.schedule([this] { _presenter.erase(*this); }, {.delay = 3s});
  return 204;
}

void DrawService::onStart() { std::cout << "draw start" << std::endl; }

void DrawService::onRenderPass(spotiled::LED &led, std::chrono::milliseconds) {
  uint8_t *data = reinterpret_cast<uint8_t *>(_request->body.data());

  if (auto it = _request->headers.find("x-draw-logo");
      it != _request->headers.end() && it->second.size() == 3) {
    uint8_t *logo = reinterpret_cast<uint8_t *>(it->second.data());
    led.setLogo({logo[0], logo[1], logo[2]});
  }

  for (auto x = 0; x < 23; ++x) {
    for (auto y = 0; y < 16; ++y) {
      auto *p = &data[(x * 16 + y) * 3];
      auto [r, g, b] = std::tie(p[0], p[1], p[2]);
      if (std::max({r, g, b}) > 0) {
        led.set({x, y}, {r, g, b});
      }
    }
  }
}

void DrawService::onStop() {
  std::cout << "draw stop" << std::endl;
  _request.reset();
}
