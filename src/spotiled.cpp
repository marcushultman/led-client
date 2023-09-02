#include "spotiled.h"

#include <array>
#include <iostream>
#include <iterator>
#include <vector>

#include "apa102.h"

Coord operator+(const Coord &lhs, const Coord &rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
Coord operator*(const Coord &lhs, const Coord &rhs) { return {lhs.x * rhs.x, lhs.y * rhs.y}; }

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

std::unique_ptr<StaticPresenter> StaticPresenter::create(SpotiLED &led, uint8_t brightness) {
  class StaticPresenterImpl final : public StaticPresenter {
   public:
    StaticPresenterImpl(SpotiLED &led, uint8_t brightness) : _led{led}, _brightness{brightness} {}

    void present(Page &page) {
      _led.clear();
      for (auto &placement : page.sprites()) {
        if (!placement.sprite) {
          continue;
        }
        for (auto &section : placement.sprite->sections) {
          for (auto pos : section.coords) {
            auto [r, g, b] = section.color * _brightness;
            _led.set(placement.pos + placement.scale * pos, section.color * _brightness);
          }
        }
      }
      _led.show();
    }

   private:
    SpotiLED &_led;
    uint8_t _brightness = 0;
  };
  return std::make_unique<StaticPresenterImpl>(led, brightness);
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
                                                           uint8_t brightness,
                                                           uint8_t logo_brightness) {
  class RollingPresenterImpl final : public RollingPresenter {
   public:
    RollingPresenterImpl(async::Scheduler &scheduler,
                         SpotiLED &led,
                         Page &page,
                         Direction direction,
                         uint8_t brightness,
                         uint8_t logo_brightness)
        : _led{led},
          _page{page},
          _direction{direction},
          _brightness{brightness},
          _logo_brightness{logo_brightness},
          _started{std::chrono::system_clock::now()},
          _render{scheduler.schedule([this] { present(_page); }, {.period = 200ms})} {}

    void present(Page &) {
      _led.clear();
      _led.setLogo({_logo_brightness, _logo_brightness, _logo_brightness});

      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() - _started);

      // todo: respect direction
      auto offset = Coord{23 - (static_cast<int>(kScrollSpeed * elapsed.count()) % (23 + 30)), 0};

      for (auto &placement : _page.sprites()) {
        if (!placement.sprite) {
          continue;
        }
        for (auto &section : placement.sprite->sections) {
          for (auto pos : section.coords) {
            auto [r, g, b] = section.color;
            _led.set(offset + placement.pos + placement.scale * pos, section.color * _brightness);
          }
        }
      }
      _led.show();
    }

   private:
    SpotiLED &_led;
    Page &_page;
    Direction _direction;
    uint8_t _brightness;
    uint8_t _logo_brightness;
    std::chrono::system_clock::time_point _started;
    async::Lifetime _render;
  };
  return std::make_unique<RollingPresenterImpl>(scheduler, led, page, direction, brightness,
                                                logo_brightness);
}