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

struct Presenter {
  virtual ~Presenter() = default;
  virtual void present(Page &) = 0;
};

struct StaticPresenter : Presenter {
  static std::unique_ptr<StaticPresenter> create(SpotiLED &, u_int8_t brightness);
};

enum class Direction {
  kVertical,
  kHorizontal,
};

struct RollingPresenter : Presenter {
  static std::unique_ptr<RollingPresenter> create(async::Scheduler &scheduler,
                                                  SpotiLED &,
                                                  Page &,
                                                  Direction,
                                                  uint8_t brightness,
                                                  uint8_t logo_brightness);
};

struct PagedPresenter : Presenter {
  static std::unique_ptr<PagedPresenter> create(SpotiLED &);
};