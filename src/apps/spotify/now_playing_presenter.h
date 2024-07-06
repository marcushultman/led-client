#pragma once

#include <memory>

#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

namespace spotify {

struct NowPlaying;

struct NowPlayingPresenter {
  virtual ~NowPlayingPresenter() = default;
  static std::unique_ptr<NowPlayingPresenter> create(spotiled::Renderer &,
                                                     present::PresenterQueue &,
                                                     const NowPlaying &);
};

}  // namespace spotify
