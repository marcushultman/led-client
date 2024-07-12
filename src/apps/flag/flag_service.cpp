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

FlagService::FlagService(present::PresenterQueue &presenter) : _presenter{presenter} {}

http::Response FlagService::handleRequest(http::Request req) {
  using namespace std::chrono_literals;

  if (req.method == http::Method::POST) {
    _presenter.add(*this, {.prio = present::Prio::kNotification, .render_period = 1s});
  } else if (req.method == http::Method::DELETE) {
    _presenter.erase(*this);
  } else {
    return 400;
  }
  return 204;
}

void FlagService::onRenderPass(spotiled::LED &led, std::chrono::milliseconds elapsed) {
  using namespace std::chrono_literals;

  if (elapsed >= 3s) {
    _presenter.erase(*this);
    return;
  }

  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 23; x++) {
      led.set({x, y}, kOlors[y * 23 + x]);
    }
  }
}
