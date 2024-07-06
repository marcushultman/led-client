#include "now_playing_presenter.h"

#include <chrono>
#include <cmath>
#include <iostream>

#include "now_playing_service.h"
#include "util/spotiled/spotiled.h"

namespace spotify {

class NowPlayingPresenterImpl final : public NowPlayingPresenter, present::Presentable {
 public:
  NowPlayingPresenterImpl(spotiled::Renderer &renderer,
                          present::PresenterQueue &presenter,
                          const NowPlaying &now_playing)
      : _renderer{renderer}, _presenter{presenter}, _now_playing{now_playing} {
    _presenter.add(*this);
  }
  ~NowPlayingPresenterImpl() { _presenter.erase(*this); }

  void onStart() {
    _alive = std::make_shared<bool>(true);
    _start = std::chrono::system_clock::now();
    _renderer.add([this, sentinel = std::weak_ptr<void>(_alive)](
                      auto &led, auto elapsed) -> std::chrono::milliseconds {
      using namespace std::chrono_literals;
      if (sentinel.expired() || (_stop.time_since_epoch().count() && _start + elapsed < _stop)) {
        return 0s;
      }
      displayScannable(led);
      return 200ms;
    });
  }
  void onStop() {
    using namespace std::chrono_literals;
    std::cout << "NowPlayingPresenter::stop()" << std::endl;
    _stop = std::chrono::system_clock::now() + 1s;
  }

 private:
  void displayScannable(spotiled::LED &led);

  spotiled::Renderer &_renderer;
  present::PresenterQueue &_presenter;
  const NowPlaying &_now_playing;
  std::shared_ptr<bool> _alive;
  std::chrono::system_clock::time_point _start, _stop;
};

std::unique_ptr<NowPlayingPresenter> NowPlayingPresenter::create(spotiled::Renderer &renderer,
                                                                 present::PresenterQueue &presenter,
                                                                 const NowPlaying &now_playing) {
  return std::make_unique<NowPlayingPresenterImpl>(renderer, presenter, now_playing);
}

void NowPlayingPresenterImpl::displayScannable(spotiled::LED &led) {
  led.setLogo(color::kWhite);

  // todo: enable bpm
  auto bpm = 0;  //_now_playing.bpm;
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

  for (auto col = 0; col < 23; ++col) {
    auto zto8 = bpm ? 0.5 + 0.25 * (1 + std::sin((4 * 2 * M_PI * col / 23.0) - M_PI / 2 +
                                                 (bpm / 60.0) * 2 * M_PI * ms / 1000.0))
                    : 1.0;

    auto start = 8 - int(std::round(_now_playing.lengths0[col] * zto8));
    auto end = 8 + int(std::round(_now_playing.lengths1[col] * zto8));

    for (auto y = start; y < end; ++y) {
      led.set({.x = col, .y = y}, color::kWhite);
    }
  }
}

}  // namespace spotify
