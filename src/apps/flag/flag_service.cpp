#include "flag_service.h"

#include "util/spotiled/spotiled.h"

namespace {

using color::kWhite;

constexpr Color kBlue = {0, 0, 0xff};
constexpr Color kRed = {0xff, 0, 0};

constexpr std::array<Color, 23 * 16> kOlors = {
    kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kBlue,
    kWhite, kBlue,  kWhite, kBlue,  kWhite, kBlue,  kWhite, kBlue,  kBlue,  kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kBlue,  kBlue,
    kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kRed,   kRed,   kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kBlue,  kBlue,  kBlue,
    kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kBlue,  kBlue,  kBlue,  kBlue,
    kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kRed,   kRed,   kRed,   kRed,   kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kBlue,  kWhite, kBlue,  kWhite, kBlue,
    kWhite, kBlue,  kWhite, kBlue,  kBlue,  kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,
    kBlue,  kBlue,  kBlue,  kBlue,  kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,
    kBlue,  kBlue,  kBlue,  kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,  kBlue,
    kBlue,  kBlue,  kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,
    kRed,   kRed,   kRed,   kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,
    kRed,   kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,
    kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kRed,   kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,
    kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite,

};

}  // namespace

FlagService::FlagService(spotiled::Renderer &renderer, present::PresenterQueue &presenter)
    : _renderer{renderer}, _presenter{presenter} {}

http::Response FlagService::handleRequest(http::Request req) {
  if (req.method == http::Method::POST) {
    using namespace std::chrono_literals;
    _expire_at = std::chrono::system_clock::now() + 3s;
    _presenter.add(*this, {.prio = present::Prio::kNotification});
  } else if (req.method == http::Method::DELETE) {
    _expire_at = {};
    _presenter.erase(*this);
  } else {
    return 400;
  }
  return 204;
}

void FlagService::onStart() {
  using ms = std::chrono::milliseconds;
  _renderer.add([this](auto &led, auto) -> ms {
    using namespace std::chrono_literals;
    if (std::chrono::system_clock::now() > _expire_at) {
      _presenter.erase(*this);
      return 0ms;
    }

    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 23; x++) {
        led.set({x, y}, kOlors[y * 23 + x]);
      }
    }

    return 1s;
  });
}

void FlagService::onStop() { _expire_at = {}; }
