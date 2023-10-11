#include "now_playing_presenter.h"

#include <chrono>

#include "now_playing_service.h"
#include "util/spotiled/spotiled.h"

namespace spotify {

class NowPlayingPresenterImpl final : public NowPlayingPresenter, present::Presentable {
 public:
  NowPlayingPresenterImpl(present::PresenterQueue &presenter,
                          settings::BrightnessProvider &brightness,
                          const NowPlaying &now_playing)
      : _presenter{presenter}, _brightness{brightness}, _now_playing{now_playing} {
    _presenter.add(*this);
  }
  ~NowPlayingPresenterImpl() { _presenter.erase(*this); }

  void start(SpotiLED &led, present::Callback) {
    _alive = std::make_shared<bool>(true);
    led.add([this, sentinel = std::weak_ptr<void>(_alive)](auto &led, auto) {
      using namespace std::chrono_literals;
      if (sentinel.expired()) {
        return 0s;
      }
      displayScannable(led);
      return 5s;
    });
  }
  void stop() { _alive.reset(); }

 private:
  void displayScannable(SpotiLED &led);

  present::PresenterQueue &_presenter;
  settings::BrightnessProvider &_brightness;
  const NowPlaying &_now_playing;
  std::shared_ptr<bool> _alive;
};

std::unique_ptr<NowPlayingPresenter> NowPlayingPresenter::create(
    present::PresenterQueue &presenter,
    settings::BrightnessProvider &brightness,
    const NowPlaying &now_playing) {
  return std::make_unique<NowPlayingPresenterImpl>(presenter, brightness, now_playing);
}

void NowPlayingPresenterImpl::displayScannable(SpotiLED &led) {
  led.setLogo(_brightness.logoBrightness());
  auto brightness = _brightness.brightness();

  for (auto col = 0; col < 23; ++col) {
    auto start = 8 - _now_playing.lengths0[col];
    auto end = 8 + _now_playing.lengths1[col];
    for (auto y = start; y < end; ++y) {
      led.set({.x = col, .y = y}, brightness);
    }
  }
}

}  // namespace spotify
