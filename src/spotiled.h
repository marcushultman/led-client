#pragma once

#include <array>
#include <iterator>
#include <utility>
#include <vector>

#include "apa102.h"
#include "apps/settings/brightness_provider.h"
#include "async/scheduler.h"
#include "present/presenter.h"
#include "util/color/color.h"

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

struct StaticPresenter {
  virtual ~StaticPresenter() = default;
  static std::unique_ptr<StaticPresenter> create(SpotiLED &,
                                                 settings::BrightnessProvider &,
                                                 Page &,
                                                 Coord offset = {},
                                                 Coord scale = kNormalScale);
};

enum class Direction {
  kVertical,
  kHorizontal,
};

struct RollingPresenter {
  virtual ~RollingPresenter() = default;
  static std::unique_ptr<RollingPresenter> create(async::Scheduler &scheduler,
                                                  SpotiLED &,
                                                  settings::BrightnessProvider &,
                                                  Page &,
                                                  Direction,
                                                  Coord offset = {},
                                                  Coord scale = kNormalScale);
};

struct PagedPresenter {
  static std::unique_ptr<PagedPresenter> create(async::Scheduler &scheduler,
                                                SpotiLED &,
                                                settings::BrightnessProvider &,
                                                Page &from,
                                                Page &to,
                                                Direction);
};
