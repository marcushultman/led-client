#include "apps/draw/draw_service.h"

#include <cassert>
#include <iostream>

namespace {

using namespace std::chrono_literals;

}  // namespace

DrawService::DrawService(present::PresenterQueue &presenter) : _presenter{presenter} {}

http::Response DrawService::handleRequest(http::Request req) {
  if (req.method != http::Method::POST || req.body.size() != 16 * 23 * 3) {
    return 400;
  }
  if (!std::exchange(_request, req)) {
    std::cout << "draw started" << std::endl;
    _presenter.add(*this, {.prio = present::Prio::kNotification});

  } else {
    std::cout << "draw updated" << std::endl;
    _presenter.notify();
  }
  _expire_at = std::chrono::system_clock::now() + 3s;
  return 204;
}

void DrawService::start(spotiled::Renderer &renderer, present::Callback callback) {
  assert(_request);
  std::cout << "draw presenting" << std::endl;
  renderer.add(
      [this, callback = std::move(callback)](auto &led, auto elapsed) -> std::chrono::milliseconds {
        if (!_request) {
          return 0s;
        }
        if (std::chrono::system_clock::now() > _expire_at) {
          _request.reset();
          callback();
          return 0s;
        }
        uint8_t *data = reinterpret_cast<uint8_t *>(_request->body.data());

        if (auto it = _request->headers.find("x-draw-logo");
            it != _request->headers.end() && it->second.size() == 3) {
          uint8_t *logo = reinterpret_cast<uint8_t *>(it->second.data());
          led.setLogo({logo[0], logo[1], logo[2]});
        }

        for (auto x = 0; x < 23; ++x) {
          for (auto y = 0; y < 16; ++y) {
            auto [r, g, b] = std::array<uint8_t, 3>{data[(x * 16 + y) * 3]};
            if (std::max({r, g, b}) > 0) {
              led.set({x, y}, {r, g, b});
            }
          }
        }

        if (auto it = _request->headers.find("x-draw-fps"); it != _request->headers.end()) {
          return std::chrono::milliseconds(1000 / std::stoi(it->second));
        }
        return 100ms;
      });
}

void DrawService::stop() {
  std::cout << "draw stop" << std::endl;
  _request.reset();
}
