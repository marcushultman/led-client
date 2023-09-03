#include "spotiled.h"

#include <array>
#include <iostream>
#include <iterator>
#include <vector>

#include "apa102.h"

Coord operator+(const Coord &lhs, const Coord &rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
Coord operator*(const Coord &lhs, const Coord &rhs) { return {lhs.x * rhs.x, lhs.y * rhs.y}; }

Color operator*(const Color &lhs, const Color &rhs) {
  return {static_cast<uint8_t>(static_cast<int>(lhs[0]) * rhs[0] / 255),
          static_cast<uint8_t>(static_cast<int>(lhs[1]) * rhs[1] / 255),
          static_cast<uint8_t>(static_cast<int>(lhs[2]) * rhs[2] / 255)};
}

Color operator*(const Color &c, uint8_t s) {
  return {static_cast<uint8_t>(static_cast<int>(c[0]) * s / 255),
          static_cast<uint8_t>(static_cast<int>(c[1]) * s / 255),
          static_cast<uint8_t>(static_cast<int>(c[2]) * s / 255)};
}

std::unique_ptr<SpotiLED> SpotiLED::create() {
  struct SpotiLEDImpl final : public SpotiLED {
    void clear() final { _led->clear(); }
    void setLogo(Color color) final {
      auto [r, g, b] = color;
      for (auto i = 0; i < 19; ++i) {
        _led->set(i, r, g, b);
      }
    }
    void set(Coord pos, Color color) final {
      if (pos.x >= 0 && pos.x < 23 && pos.y >= 0 && pos.y < 16) {
        auto [r, g, b] = color;
        _led->set(19 + offset(pos), r, g, b);
      }
    }
    void show() final { _led->show(); }

   private:
    size_t offset(Coord pos) { return 16 * pos.x + 15 - pos.y; }

    std::unique_ptr<apa102::LED> _led = apa102::createLED(19 + 16 * 23);
  };
  return std::make_unique<SpotiLEDImpl>();
}

// static_presenter.cpp

std::unique_ptr<StaticPresenter> StaticPresenter::create(SpotiLED &led,
                                                         Page &page,
                                                         ColorProvider brightness,
                                                         ColorProvider logo_brightness) {
  class StaticPresenterImpl final : public StaticPresenter {
   public:
    StaticPresenterImpl(SpotiLED &led,
                        Page &page,
                        ColorProvider brightness,
                        ColorProvider logo_brightness)
        : _led{led},
          _page{page},
          _brightness{std::move(brightness)},
          _logo_brightness{std::move(logo_brightness)} {}

    void present() {
      auto brightness = _brightness();

      _led.clear();
      for (auto &placement : _page.sprites()) {
        if (!placement.sprite) {
          continue;
        }
        for (auto &section : placement.sprite->sections) {
          for (auto pos : section.coords) {
            auto [r, g, b] = section.color * brightness;
            _led.set(placement.pos + placement.scale * pos, section.color * brightness);
          }
        }
      }
      _led.show();
    }

   private:
    SpotiLED &_led;
    Page &_page;
    ColorProvider _brightness;
    ColorProvider _logo_brightness;
  };
  return std::make_unique<StaticPresenterImpl>(led, page, std::move(brightness),
                                               std::move(logo_brightness));
};

// rolling_presenter.cpp

namespace {

using namespace std::chrono_literals;

const auto kScrollSpeed = 0.005;

}  // namespace

std::unique_ptr<RollingPresenter> RollingPresenter::create(async::Scheduler &scheduler,
                                                           SpotiLED &led,
                                                           Page &page,
                                                           Direction direction,
                                                           ColorProvider brightness,
                                                           ColorProvider logo_brightness) {
  class RollingPresenterImpl final : public RollingPresenter {
   public:
    RollingPresenterImpl(async::Scheduler &scheduler,
                         SpotiLED &led,
                         Page &page,
                         Direction direction,
                         ColorProvider brightness,
                         ColorProvider logo_brightness)
        : _led{led},
          _page{page},
          _direction{direction},
          _brightness{std::move(brightness)},
          _logo_brightness{std::move(logo_brightness)},
          _started{std::chrono::system_clock::now()},
          _render{scheduler.schedule([this] { present(); }, {.period = 200ms})} {}

    void present() {
      _led.clear();
      _led.setLogo(_logo_brightness());

      auto brightness = _brightness();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() - _started);

      auto width = 0;
      for (auto &placement : _page.sprites()) {
        width = std::max(width, placement.pos.x + (placement.sprite ? placement.sprite->width : 0));
      }

      // todo: respect direction
      auto offset =
          Coord{23 - (static_cast<int>(kScrollSpeed * elapsed.count()) % (23 + width)), 0};

      for (auto &placement : _page.sprites()) {
        if (!placement.sprite) {
          continue;
        }
        for (auto &section : placement.sprite->sections) {
          for (auto pos : section.coords) {
            auto [r, g, b] = section.color;
            _led.set(offset + placement.pos + placement.scale * pos, section.color * brightness);
          }
        }
      }
      _led.show();
    }

   private:
    SpotiLED &_led;
    Page &_page;
    Direction _direction;
    ColorProvider _brightness;
    ColorProvider _logo_brightness;
    std::chrono::system_clock::time_point _started;
    async::Lifetime _render;
  };
  return std::make_unique<RollingPresenterImpl>(scheduler, led, page, direction,
                                                std::move(brightness), std::move(logo_brightness));
}
