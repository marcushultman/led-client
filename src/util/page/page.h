#pragma once

#include <vector>

#include "util/gfx/gfx.h"

namespace page {

struct Page {
  struct SpritePlacement {
    Coord pos;
    const Sprite *sprite = nullptr;
    Coord scale = kNormalScale;
  };

  virtual ~Page() = default;
  virtual const std::vector<SpritePlacement> &sprites() = 0;
};

}  // namespace page
