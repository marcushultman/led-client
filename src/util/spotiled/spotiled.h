#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include "async/scheduler.h"
#include "util/apa102/apa102.h"
#include "util/color/color.h"
#include "util/gfx/gfx.h"

struct SpotiLED {
  using Options = apa102::SetOptions;

  virtual ~SpotiLED() = default;
  virtual void setLogo(Color, const Options &options = {}) = 0;
  virtual void set(Coord pos, Color, const Options &options = {}) = 0;

  using RenderCallback =
      std::function<std::chrono::milliseconds(SpotiLED &, std::chrono::milliseconds elapsed)>;
  virtual void add(RenderCallback) = 0;
  virtual void notify() = 0;

  static std::unique_ptr<SpotiLED> create(async::Scheduler &main_scheduler);
};
