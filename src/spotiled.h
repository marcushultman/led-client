#pragma once

#include <array>
#include <iterator>
#include <vector>

#include "apa102.h"
#include "async/scheduler.h"

struct Coord {
  int x = 0;
  int y = 0;
};

Coord operator+(const Coord &lhs, const Coord &rhs);
Coord operator*(const Coord &lhs, const Coord &rhs);

using Color = std::array<uint8_t, 3>;

Color operator*(const Color &, const Color &);
Color operator*(const Color &c, uint8_t s);

constexpr auto kBlack = Color{0, 0, 0};
constexpr auto kWhite = Color{255, 255, 255};

struct Section {
  Color color;
  std::vector<Coord> coords;
};

struct Sprite {
  int width = 0;
  std::vector<Section> sections;
};

//

constexpr auto kNormalScale = Coord{1, 1};

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

using ColorProvider = std::function<Color()>;

struct StaticPresenter {
  virtual ~StaticPresenter() = default;
  static std::unique_ptr<StaticPresenter> create(SpotiLED &,
                                                 Page &,
                                                 ColorProvider brightness,
                                                 ColorProvider logo_brightness);
};

enum class Direction {
  kVertical,
  kHorizontal,
};

struct RollingPresenter {
  virtual ~RollingPresenter() = default;
  static std::unique_ptr<RollingPresenter> create(async::Scheduler &scheduler,
                                                  SpotiLED &,
                                                  Page &,
                                                  Direction,
                                                  ColorProvider brightness,
                                                  ColorProvider logo_brightness);
};

struct PagedPresenter {
  static std::unique_ptr<PagedPresenter> create(SpotiLED &);
};
