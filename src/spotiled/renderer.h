#pragma once

#include <color/color.h>
#include <led/led.h>

#include <chrono>
#include <functional>

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
  using Options = led::SetOptions;

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
};

}  // namespace spotiled
