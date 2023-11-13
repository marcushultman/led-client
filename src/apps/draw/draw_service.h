#pragma once

#include <cassert>
#include <charconv>
#include <iostream>
#include <queue>

#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"

struct DrawService : present::Presentable {
  explicit DrawService(present::PresenterQueue &presenter) : _presenter{presenter} {}

  http::Response handleRequest(http::Request req) {
    if (req.method != http::Method::POST || req.body.size() != 16 * 23 * 3) {
      std::cout << "draw 400" << std::endl;
      return 400;
    }
    using namespace std::chrono_literals;
    if (!std::exchange(_request, req)) {
      std::cout << "draw started: " << req.body.size() << std::endl;
      _presenter.add(*this, {.prio = present::Prio::kNotification});
      _expire_at = std::chrono::system_clock::now() + 5s;
    } else {
      std::cout << "draw updated: " << req.body.size() << std::endl;
      _presenter.notify();
      _expire_at += 1s;
    }
    return 204;
  }

  void start(spotiled::Renderer &renderer, present::Callback callback) final {
    assert(_request);
    std::cout << "present start" << std::endl;
    renderer.add([this, callback = std::move(callback)](spotiled::LED &led,
                                                        auto elapsed) -> std::chrono::milliseconds {
      using namespace std::chrono_literals;
      if (!_request) {
        std::cout << "render: no req" << std::endl;
        return 0s;
      }
      if (std::chrono::system_clock::now() > _expire_at) {
        std::cout << "render: expired" << std::endl;
        stop();
        callback();
        return 0s;
      }
      uint8_t *data = reinterpret_cast<uint8_t *>(_request->body.data());

      auto sum = 0;

      for (auto x = 0; x < 23; ++x) {
        for (auto y = 0; y < 16; ++y) {
          auto *rgb = &data[(x * 16 + y) * 3];
          if (std::max({rgb[0], rgb[1], rgb[2]}) > 0) {
            led.set({x, y}, {rgb[0], rgb[1], rgb[2]});
          }
          sum += rgb[0] + rgb[1] + rgb[2];
        }
      }
      std::cout << "render!" << int(sum) << std::endl;
      return 100ms;
    });
  }
  void stop() final {
    std::cout << "stop" << std::endl;
    _request.reset();
    _expire_at = {};
  }

 private:
  present::PresenterQueue &_presenter;
  std::optional<http::Request> _request;
  std::chrono::system_clock::time_point _expire_at;
};
