#include <array>
#include <iostream>
#include <map>

#include "font/font.h"
#include "util/apa102/apa102.h"
#include "util/spotiled/brightness_provider.h"

namespace weather {
using namespace color;

// todo: access from weather
const auto kAlphaMap = std::map<char, Sprite>{};

auto kDegrees = Sprite{.sections = {{.color = kWhite, .coords = {{0, 1}, {1, 0}, {1, 2}, {2, 1}}}}};

auto kSun = Sprite{.width = 9,
                   .sections = {{.color = {255, 255, 0},
                                 .coords = {
                                     {0, 5},                                   //
                                     {1, 1}, {1, 5}, {1, 9},                   //
                                     {2, 2}, {2, 8},                           //
                                     {3, 4}, {3, 5}, {3, 6},                   //
                                     {4, 0}, {4, 1}, {4, 3}, {4, 4},  {4, 5},  //
                                     {4, 6}, {4, 7}, {4, 9}, {4, 10},          //
                                     {5, 4}, {5, 5}, {5, 6},                   //
                                     {6, 2}, {6, 8},                           //
                                     {7, 1}, {7, 5}, {7, 9},                   //
                                     {8, 5},                                   //
                                 }}}};

auto kCloud = Sprite{.width = 9,
                     .sections = {{.color = kWhite,
                                   .coords = {
                                       {0, 4}, {0, 5}, {0, 6}, {0, 7},                          //
                                       {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {1, 8},          //
                                       {2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8},  //
                                       {3, 2}, {3, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 7}, {3, 8},  //
                                       {4, 3}, {4, 4}, {4, 5}, {4, 6}, {4, 7}, {4, 8},          //
                                       {5, 4}, {5, 5}, {5, 6}, {5, 7}, {5, 8},                  //
                                       {6, 3}, {6, 4}, {6, 5}, {6, 6}, {6, 7}, {6, 8},          //
                                       {7, 4}, {7, 5}, {7, 6}, {7, 7}, {7, 8},                  //
                                       {8, 5}, {8, 6}, {8, 7},                                  //
                                   }}}};

auto kRain =
    Sprite{.width = 9,
           .sections = {
               {.color = kWhite,
                .coords =
                    {
                        {0, 3}, {0, 4}, {0, 5}, {0, 6},                          //
                        {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7},          //
                        {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7},  //
                        {3, 1}, {3, 2}, {3, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 7},  //
                        {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}, {4, 7},          //
                        {5, 3}, {5, 4}, {5, 5}, {5, 6}, {5, 7},                  //
                        {6, 2}, {6, 3}, {6, 4}, {6, 5}, {6, 6}, {6, 7},          //
                        {7, 3}, {7, 4}, {7, 5}, {7, 6}, {7, 7},                  //
                        {8, 4}, {8, 5}, {8, 6},                                  //
                    }},
               {.color = {0, 0, 255}, .coords = {{2, 8}, {2, 9}, {4, 8}, {4, 9}, {6, 8}, {6, 9}}}}};

struct WeatherPageImpl final : page::Page {
  const std::vector<SpritePlacement> &sprites() final { return _sprites; }

  void setText(const std::string &text) {
    _sprites.clear();
    _sprites.reserve(text.size() + 2);

    _sprites.push_back({.pos = {0, 2},
                        .sprite = text.size() && text[0] == '1'
                                      ? &kSun
                                      : text.size() && text[0] == '2' ? &kCloud : &kRain});

    auto x = 10;

    for (auto n = 0; n < text.size(); ++n) {
      auto it = kAlphaMap.find(text[n]);
      if (it == kAlphaMap.end()) {
        continue;
      }
      auto &glyph = it->second;
      _sprites.push_back({.pos = {x, 3}, .sprite = &glyph, .scale = {1, 2}});
      x += glyph.width + 1;
    }
    _sprites.push_back({.pos = {x, 3}, .sprite = &kDegrees});
  }

 private:
  std::vector<SpritePlacement> _sprites;
};

}  // namespace weather
