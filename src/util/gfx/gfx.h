#pragma once

#include <vector>

#include "util/color/color.h"

struct Coord {
  int x = 0;
  int y = 0;
};

Coord operator+(const Coord &lhs, const Coord &rhs);
Coord operator*(const Coord &lhs, const Coord &rhs);

constexpr auto kNormalScale = Coord{1, 1};

struct Section {
  Color color;
  std::vector<Coord> coords;
};

struct Sprite {
  int width = 0;
  std::vector<Section> sections;
};
