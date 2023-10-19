#pragma once

#include "async/scheduler.h"
#include "util/color/color.h"
#include "util/gfx/gfx.h"
#include "util/page/page.h"
#include "util/spotiled/spotiled.h"

enum class Direction {
  kVertical,
  kHorizontal,
};

void renderRolling(spotiled::LED &led,
                   std::chrono::milliseconds elapsed,
                   page::Page &page,
                   Coord offset = {},
                   Coord scale = kNormalScale,
                   double speed = 0.005);
