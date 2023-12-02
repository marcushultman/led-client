#pragma once

#include <vector>

#include "util/color/color.h"

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

constexpr auto kNormalScale = Coord{1, 1};

struct Section {
  Color color;
  std::vector<Coord> coords;
};

struct Sprite {
  int width = 0;
  std::vector<Section> sections;
};
