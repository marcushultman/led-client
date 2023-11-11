#include "flag_service.h"

#include <queue>

#include "present/rolling_presenter.h"
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

constexpr auto kDelayHeader = "delay";
constexpr auto kDefaultTimeout = std::chrono::seconds{5};
constexpr auto kSpeedHeader = "speed";

std::chrono::milliseconds timeout(const http::Request &req) {
  if (auto it = req.headers.find(kDelayHeader); it != req.headers.end()) {
    int delay_s = 0;
    auto value = std::string_view(it->second);
    std::from_chars(value.begin(), value.end(), delay_s);
    return std::chrono::seconds(delay_s);
  }
  return kDefaultTimeout;
}

double speed(const http::Request &req) {
  if (auto it = req.headers.find(kSpeedHeader); it != req.headers.end()) {
    return std::stod(it->second);
  }
  return 0.005;
}

}  // namespace

FlagService::FlagService(async::Scheduler &main_scheduler, present::PresenterQueue &presenter)
    : _main_scheduler{main_scheduler}, _presenter{presenter} {}

http::Response FlagService::handleRequest(http::Request) {
  _presenter.add(*this, {.prio = present::Prio::kNotification});

  return 204;
}

void FlagService::start(spotiled::Renderer &renderer, present::Callback callback) {
  // Få tag i flaggans data

  _sentinel = std::make_shared<bool>(true);
  renderer.add([this, callback = std::move(callback), alive = std::weak_ptr<void>(_sentinel)](
                   spotiled::LED &led, auto elapsed) {
    using namespace std::chrono_literals;
    if (alive.expired()) {
      callback();
      return 0ms;
    }

    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 23; x++) {
        led.set({x, y}, kOlors[y * 23 + x]);
      }
    }

    return 100ms;
  });
}

void FlagService::stop() {
  _sentinel.reset();
  _presenter.notify();
}
