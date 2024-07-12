#include "now_playing_presenter.h"

#include <chrono>
#include <cmath>

#include "now_playing_service.h"
#include "util/spotiled/spotiled.h"

namespace spotify {

class NowPlayingPresenterImpl final : public NowPlayingPresenter, present::Presentable {
 public:
  NowPlayingPresenterImpl(present::PresenterQueue &presenter, const NowPlaying &now_playing)
      : _presenter{presenter}, _now_playing{now_playing} {
    using namespace std::chrono_literals;

    _presenter.add(*this, {.render_period = 200ms});
  }
  ~NowPlayingPresenterImpl() { _presenter.erase(*this); }

 private:
  void onRenderPass(spotiled::LED &led, std::chrono::milliseconds elapsed) final;

  present::PresenterQueue &_presenter;
  const NowPlaying &_now_playing;
};

std::unique_ptr<NowPlayingPresenter> NowPlayingPresenter::create(present::PresenterQueue &presenter,
                                                                 const NowPlaying &now_playing) {
  return std::make_unique<NowPlayingPresenterImpl>(presenter, now_playing);
}

void NowPlayingPresenterImpl::onRenderPass(spotiled::LED &led, std::chrono::milliseconds elapsed) {
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
