#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include "async/scheduler.h"
#include "brightness_provider.h"
#include "util/apa102/apa102.h"
#include "util/color/color.h"

namespace spotiled {

struct Coord {
  int x = 0;
  int y = 0;
};

constexpr Coord operator+(const Coord &lhs, const Coord &rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}
constexpr Coord operator*(const Coord &lhs, const Coord &rhs) {
  return {lhs.x * rhs.x, lhs.y * rhs.y};
}
constexpr Coord operator/(const Coord &lhs, const Coord &rhs) {
  return {lhs.x / rhs.x, lhs.y / rhs.y};
}

struct LED {
  using Options = apa102::SetOptions;
  virtual ~LED() = default;
  virtual void setLogo(Color, const Options & = {}) = 0;
  virtual void set(Coord, Color, const Options & = {}) = 0;
};

struct Renderer {
  using RenderCallback =
      std::function<std::chrono::milliseconds(LED &, std::chrono::milliseconds elapsed)>;

  virtual ~Renderer() = default;
  virtual void add(RenderCallback) = 0;
  virtual void notify() = 0;

  static std::unique_ptr<Renderer> create(async::Scheduler &main_scheduler, BrightnessProvider &);
};

}  // namespace spotiled
