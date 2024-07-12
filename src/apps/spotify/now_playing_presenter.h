#pragma once

#include <memory>

#include "present/presenter.h"

namespace spotify {

struct NowPlaying;

struct NowPlayingPresenter {
  virtual ~NowPlayingPresenter() = default;
  static std::unique_ptr<NowPlayingPresenter> create(present::PresenterQueue &, const NowPlaying &);
};

}  // namespace spotify
