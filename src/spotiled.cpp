#include "spotiled.h"

#include <array>
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
      auto [r, g, b] = color;
      _led->set(19 + offset(pos), r, g, b);
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
        for (auto &section : placement.sprite.sections) {
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

std::unique_ptr<RollingPresenter> RollingPresenter::create(SpotiLED &led,
                                                           Direction direction,
                                                           uint8_t brightness,
                                                           uint8_t logo_brightness) {
  class RollingPresenterImpl final : public RollingPresenter {
   public:
    RollingPresenterImpl(SpotiLED &led,
                         Direction direction,
                         uint8_t brightness,
                         uint8_t logo_brightness)
        : _led{led},
          _direction{direction},
          _brightness{brightness},
          _logo_brightness(logo_brightness) {}

    void present(Page &page) {
      _led.clear();
      _led.setLogo({_logo_brightness, _logo_brightness, _logo_brightness});

      const auto kScrollSpeed = 0.005;
      auto offset = Coord{23 - (static_cast<int>(kScrollSpeed * _elapsed.count()) % (23 + 30)), 0};

      for (auto &placement : page.sprites()) {
        for (auto &section : placement.sprite.sections) {
          for (auto pos : section.coords) {
            auto [r, g, b] = section.color;
            auto pixel_pos = offset + placement.pos + placement.scale * pos;
            if (pixel_pos.x < 0) {
              continue;
            }
            _led.set(pixel_pos, section.color * _brightness);
          }
        }
      }
      _led.show();
    }

    void HACK_setElapsed(std::chrono::milliseconds elapsed) { _elapsed = elapsed; }

   private:
    SpotiLED &_led;
    Direction _direction;
    uint8_t _brightness;
    uint8_t _logo_brightness;

    // calculate this internally using threads
    std::chrono::milliseconds _elapsed;
  };
  return std::make_unique<RollingPresenterImpl>(led, direction, brightness, logo_brightness);
}
