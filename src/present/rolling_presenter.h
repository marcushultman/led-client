#pragma once

#include "apps/settings/brightness_provider.h"
#include "async/scheduler.h"
#include "util/color/color.h"
#include "util/gfx/gfx.h"
#include "util/page/page.h"
#include "util/spotiled/spotiled.h"

enum class Direction {
  kVertical,
  kHorizontal,
};

struct RollingPresenter {
  virtual ~RollingPresenter() = default;
  static std::unique_ptr<RollingPresenter> create(async::Scheduler &scheduler,
                                                  SpotiLED &,
                                                  settings::BrightnessProvider &,
                                                  page::Page &,
                                                  Direction,
                                                  Coord offset = {},
                                                  Coord scale = kNormalScale);
};
