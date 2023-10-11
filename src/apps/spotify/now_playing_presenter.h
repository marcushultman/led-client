#pragma once

#include <memory>

#include "apps/settings/brightness_provider.h"
#include "present/presenter.h"

namespace spotify {

struct NowPlaying;

struct NowPlayingPresenter {
  virtual ~NowPlayingPresenter() = default;
  static std::unique_ptr<NowPlayingPresenter> create(present::PresenterQueue &,
                                                     settings::BrightnessProvider &,
                                                     const NowPlaying &);
};

}  // namespace spotify
