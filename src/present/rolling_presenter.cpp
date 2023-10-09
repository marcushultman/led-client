#include "rolling_presenter.h"

#include <array>
#include <iostream>
#include <iterator>
#include <vector>

#include "apps/settings/brightness_provider.h"
#include "util/apa102/apa102.h"
#include "util/gfx/gfx.h"
#include "util/spotiled/spotiled.h"

namespace {

using namespace std::chrono_literals;

const auto kScrollSpeed = 0.005;

}  // namespace

void renderRolling(SpotiLED &led,
                   const settings::BrightnessProvider &brightness_provider,
                   std::chrono::milliseconds elapsed,
                   page::Page &page,
                   Coord offset,
                   Coord scale) {
  led.setLogo(brightness_provider.logoBrightness());

  auto brightness = brightness_provider.brightness();

  auto width = 0;
  for (auto &placement : page.sprites()) {
    width = std::max(width, placement.pos.x + (placement.sprite ? placement.sprite->width : 0));
  }

  // todo: respect direction
  auto scroll_offset =
      Coord{23 - (static_cast<int>(kScrollSpeed * elapsed.count()) % (23 + width)), 0};

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

std::unique_ptr<RollingPresenter> RollingPresenter::create(async::Scheduler &scheduler,
                                                           SpotiLED &led,
                                                           settings::BrightnessProvider &brightness,
                                                           page::Page &page,
                                                           Direction direction,
                                                           Coord offset,
                                                           Coord scale) {
  class RollingPresenterImpl final : public RollingPresenter {
   public:
    RollingPresenterImpl(async::Scheduler &scheduler,
                         SpotiLED &led,
                         settings::BrightnessProvider &brightness,
                         page::Page &page,
                         Direction direction,
                         Coord offset,
                         Coord scale)
        : _led{led},
          _brightness{brightness},
          _page{page},
          _direction{direction},
          _offset{offset},
          _scale{scale},
          _started{std::chrono::system_clock::now()},
          _render{scheduler.schedule([this] { present(); }, {.period = 200ms})} {}

    void present() {
      _led.clear();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() - _started);
      renderRolling(_led, _brightness, elapsed, _page, _offset, _scale);
      _led.show();
    }

   private:
    SpotiLED &_led;
    settings::BrightnessProvider &_brightness;
    page::Page &_page;
    Direction _direction;
    Coord _offset;
    Coord _scale;
    std::chrono::system_clock::time_point _started;
    async::Lifetime _render;
  };
  return std::make_unique<RollingPresenterImpl>(scheduler, led, brightness, page, direction, offset,
                                                scale);
}
