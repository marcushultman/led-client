#include "spotiled.h"

#include <array>
#include <iostream>
#include <iterator>
#include <vector>

#include "apa102.h"
#include "time_of_day_brightness.h"

Coord operator+(const Coord &lhs, const Coord &rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
Coord operator*(const Coord &lhs, const Coord &rhs) { return {lhs.x * rhs.x, lhs.y * rhs.y}; }

std::unique_ptr<BrightnessProvider> BrightnessProvider::create(uint8_t brightness) {
  struct BrightnessProviderImpl final : public BrightnessProvider {
    explicit BrightnessProviderImpl(uint8_t brightness) : _brightness{brightness} {}
    Color logoBrightness() const final { return timeOfDayBrightness(_brightness); }
    Color brightness() const final { return timeOfDayBrightness(3 * _brightness / 4); }

    uint8_t _brightness = 0;
  };
  return std::make_unique<BrightnessProviderImpl>(brightness);
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
                                                         BrightnessProvider &brightness,
                                                         Page &page,
                                                         Coord offset) {
  class StaticPresenterImpl final : public StaticPresenter {
   public:
    StaticPresenterImpl(SpotiLED &led, BrightnessProvider &brightness, Page &page, Coord offset)
        : _led{led}, _brightness{brightness}, _page{page}, _offset{offset} {}

    void present() {
      _led.clear();
      _led.setLogo(_brightness.logoBrightness());

      auto brightness = _brightness.brightness();

      for (auto &placement : _page.sprites()) {
        if (!placement.sprite) {
          continue;
        }
        for (auto &section : placement.sprite->sections) {
          for (auto pos : section.coords) {
            _led.set(_offset + placement.pos + placement.scale * pos, section.color * brightness);
          }
        }
      }
      _led.show();
    }

   private:
    SpotiLED &_led;
    BrightnessProvider &_brightness;
    Page &_page;
    Coord _offset;
  };
  return std::make_unique<StaticPresenterImpl>(led, brightness, page, offset);
};

// rolling_presenter.cpp

namespace {

using namespace std::chrono_literals;

const auto kScrollSpeed = 0.005;

}  // namespace

std::unique_ptr<RollingPresenter> RollingPresenter::create(async::Scheduler &scheduler,
                                                           SpotiLED &led,
                                                           BrightnessProvider &brightness,
                                                           Page &page,
                                                           Direction direction,
                                                           Coord offset) {
  class RollingPresenterImpl final : public RollingPresenter {
   public:
    RollingPresenterImpl(async::Scheduler &scheduler,
                         SpotiLED &led,
                         BrightnessProvider &brightness,
                         Page &page,
                         Direction direction,
                         Coord offset)
        : _led{led},
          _brightness{brightness},
          _page{page},
          _direction{direction},
          _offset{offset},
          _started{std::chrono::system_clock::now()},
          _render{scheduler.schedule([this] { present(); }, {.period = 200ms})} {}

    void present() {
      _led.clear();
      _led.setLogo(_brightness.logoBrightness());

      auto brightness = _brightness.brightness();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() - _started);

      auto width = 0;
      for (auto &placement : _page.sprites()) {
        width = std::max(width, placement.pos.x + (placement.sprite ? placement.sprite->width : 0));
      }

      // todo: respect direction
      auto scroll_offset =
          Coord{23 - (static_cast<int>(kScrollSpeed * elapsed.count()) % (23 + width)), 0};

      for (auto &placement : _page.sprites()) {
        if (!placement.sprite) {
          continue;
        }
        for (auto &section : placement.sprite->sections) {
          for (auto pos : section.coords) {
            _led.set(_offset + scroll_offset + placement.pos + placement.scale * pos,
                     section.color * brightness);
          }
        }
      }
      _led.show();
    }

   private:
    SpotiLED &_led;
    BrightnessProvider &_brightness;
    Page &_page;
    Direction _direction;
    Coord _offset;
    std::chrono::system_clock::time_point _started;
    async::Lifetime _render;
  };
  return std::make_unique<RollingPresenterImpl>(scheduler, led, brightness, page, direction,
                                                offset);
}
