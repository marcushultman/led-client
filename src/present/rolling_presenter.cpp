#include "rolling_presenter.h"

#include <array>
#include <iostream>
#include <iterator>
#include <vector>

#include "util/apa102/apa102.h"
#include "util/gfx/gfx.h"
#include "util/spotiled/spotiled.h"

void renderRolling(spotiled::LED &led,
                   std::chrono::milliseconds elapsed,
                   page::Page &page,
                   Coord offset,
                   Coord scale,
                   double speed) {
  led.setLogo(color::kWhite);

  auto width = 0;
  for (auto &placement : page.sprites()) {
    width = std::max(width, placement.pos.x + (placement.sprite ? placement.sprite->width : 0));
  }

  // todo: respect direction
  auto scroll_offset = Coord{23 - (static_cast<int>(speed * elapsed.count()) % (23 + width)), 0};

  for (auto &placement : page.sprites()) {
    if (!placement.sprite) {
      continue;
    }
    for (auto &section : placement.sprite->sections) {
      for (auto pos : section.coords) {
        led.set(offset + scale * (scroll_offset + placement.pos + placement.scale * pos),
                section.color);
      }
    }
  }
}
