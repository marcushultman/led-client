#include "gfx.h"

Coord operator+(const Coord &lhs, const Coord &rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
Coord operator*(const Coord &lhs, const Coord &rhs) { return {lhs.x * rhs.x, lhs.y * rhs.y}; }
