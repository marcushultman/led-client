#include <vector>
#include <stdio.h>
#include "apa102.h"

std::pair<int, int> r(int col, int upper, int lower) {
  auto odd = col % 2 == 1;
  auto start = odd ? -upper : -lower;
  auto end = odd ? lower : upper;
  auto mid = 16 * col + 8;
  return {mid + start, mid + end};
}

// clang-format off
const auto ranges = std::vector<std::pair<int, int>>{
  r(0, 1, 1),
  r(1, 7, 7),
  r(2, 1, 1),
  r(3, 3, 3),
  r(4, 5, 5),
  r(5, 6, 6),
  r(6, 2, 2),
  r(7, 5, 5),
  r(8, 6, 6),
  r(9, 3, 3),
  r(10, 4, 4),
  r(11, 8, 8),
  r(12, 4, 4),
  r(13, 8, 8),
  r(14, 3, 3),
  r(15, 6, 6),
  r(16, 7, 7),
  r(17, 3, 3),
  r(18, 6, 6),
  r(19, 8, 8),
  r(20, 5, 5),
  r(21, 4, 4),
  r(22, 1, 1),
};
// clang-format on

int main() {
  auto led = apa102::LED(16 * 23);
  for (auto &[start, end] : ranges) {
    for (auto i = start; i < end; ++i) {
      led.set(i, 1, 1, 1);
    }
  }
  led.show();
}
