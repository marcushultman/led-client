#pragma once

#include <memory>

#include "util/color/color.h"
#include "util/gfx/gfx.h"

struct SpotiLED {
  virtual ~SpotiLED() = default;
  virtual void clear() = 0;
  virtual void setLogo(Color) = 0;
  virtual void set(Coord pos, Color) = 0;
  virtual void blend(Coord pos, Color, float blend = 0.5F) = 0;
  virtual void show() = 0;

  static std::unique_ptr<SpotiLED> create();
};
