#pragma once

#include <array>
#include <iterator>
#include <utility>
#include <vector>

#include "apa102.h"
#include "async/scheduler.h"
#include "util/color.h"

struct Coord {
  int x = 0;
  int y = 0;
};

Coord operator+(const Coord &lhs, const Coord &rhs);
Coord operator*(const Coord &lhs, const Coord &rhs);

constexpr auto kNormalScale = Coord{1, 1};

//

struct Section {
  Color color;
  std::vector<Coord> coords;
};

struct Sprite {
  int width = 0;
  std::vector<Section> sections;
};

//

struct Page {
  struct SpritePlacement {
    Coord pos;
    const Sprite *sprite = nullptr;
    Coord scale = kNormalScale;
  };

  virtual ~Page() = default;
  virtual const std::vector<SpritePlacement> &sprites() = 0;
};

//

struct SpotiLED {
  virtual ~SpotiLED() = default;
  virtual void clear() = 0;
  virtual void setLogo(Color) = 0;
  virtual void set(Coord pos, Color) = 0;
  virtual void show() = 0;

  static std::unique_ptr<SpotiLED> create();
};

//

struct BrightnessProvider {
  virtual ~BrightnessProvider() = default;
  virtual Color logoBrightness() const = 0;
  virtual Color brightness() const = 0;

  static std::unique_ptr<BrightnessProvider> create(uint8_t brightness);
};

//

struct StaticPresenter {
  virtual ~StaticPresenter() = default;
  static std::unique_ptr<StaticPresenter> create(
      SpotiLED &, BrightnessProvider &, Page &, Coord offset = {}, Coord scale = kNormalScale);
};

enum class Direction {
  kVertical,
  kHorizontal,
};

struct RollingPresenter {
  virtual ~RollingPresenter() = default;
  static std::unique_ptr<RollingPresenter> create(async::Scheduler &scheduler,
                                                  SpotiLED &,
                                                  BrightnessProvider &,
                                                  Page &,
                                                  Direction,
                                                  Coord offset = {},
                                                  Coord scale = kNormalScale);
};

struct PagedPresenter {
  static std::unique_ptr<PagedPresenter> create(async::Scheduler &scheduler,
                                                SpotiLED &,
                                                BrightnessProvider &,
                                                Page &from,
                                                Page &to,
                                                Direction);
};
