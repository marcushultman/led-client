#include "rolling_presenter.h"

#include <array>
#include <iostream>
#include <iterator>
#include <vector>

#include "apps/settings/brightness_provider.h"
#include "util/apa102/apa102.h"
#include "util/gfx/gfx.h"
#include "util/spotiled/spotiled.h"

void renderRolling(SpotiLED &led,
                   const settings::BrightnessProvider &brightness_provider,
                   std::chrono::milliseconds elapsed,
                   page::Page &page,
                   Coord offset,
                   Coord scale,
                   double speed) {
  led.setLogo(brightness_provider.logoBrightness());

  auto brightness = brightness_provider.brightness();

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
                section.color * brightness);
      }
    }
  }
}
