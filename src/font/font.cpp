#include "font.h"

#include <array>
#include <iostream>
#include <map>

#include "apa102.h"

namespace font {

const auto kAlphaMap = std::map<char, Sprite>{
    {'A',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1},
                               {0, 2},
                               {0, 3},
                               {0, 4},
                               {1, 0},
                               {1, 3},
                               {2, 1},
                               {2, 2},
                               {2, 3},
                               {2, 4}}}}}},
    {'B',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 0},
                               {0, 1},
                               {0, 2},
                               {0, 3},
                               {0, 4},
                               {1, 0},
                               {1, 2},
                               {1, 4},
                               {2, 1},
                               {2, 3}}}}}},
    {'C',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1}, {0, 2}, {0, 3}, {1, 0}, {1, 4}, {2, 0}, {2, 4}}}}}},
    {'D',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 0},
                         {1, 4},
                         {2, 1},
                         {2, 2},
                         {2, 3}}}}}},
    {'E',
     {.width = 2,
      .sections = {{.color = kWhite,
                    .coords = {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 0}, {1, 2}, {1, 4}}}}}},
    {'F',
     {.width = 2,
      .sections = {{.color = kWhite,
                    .coords = {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 0}, {1, 2}}}}}},
    {'G',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 0},
                         {1, 4},
                         {2, 0},
                         {2, 3},
                         {2, 4}}}}}},
    {'H',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 2},
                         {2, 0},
                         {2, 1},
                         {2, 2},
                         {2, 3},
                         {2, 4}}}}}},
    {'I',
     {.width = 3,
      .sections =
          {{.color = kWhite,
            .coords = {{0, 0}, {0, 4}, {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {2, 0}, {2, 4}}}}}},
    {'J',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 3},
                         {0, 4},
                         {1, 0},
                         {1, 4},
                         {2, 0},
                         {2, 1},
                         {2, 2},
                         {2, 3},
                         {2, 4}}}}}},
    {'K',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 2},
                         {2, 0},
                         {2, 1},
                         {2, 3},
                         {2, 4}}}}}},
    {'L',
     {.width = 2,
      .sections = {{.color = kWhite, .coords = {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 4}}}}}},
    {'M',
     {.width = 5,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 0},
                         {2, 1},
                         {2, 2},
                         {3, 0},
                         {4, 1},
                         {4, 2},
                         {4, 3},
                         {4, 4}}}}}},
    {'N',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 1},
                         {2, 2},
                         {3, 0},
                         {3, 1},
                         {3, 2},
                         {3, 3},
                         {3, 4}}}}}},
    {'O',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 1},
                         {0, 2},
                         {0, 3},
                         {1, 0},
                         {1, 4},
                         {2, 0},
                         {2, 4},
                         {3, 1},
                         {3, 2},
                         {3, 3}}}}}},
    {'P',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 0},
                         {1, 2},
                         {2, 0},
                         {2, 2},
                         {3, 1}}}}}},
    {'Q',
     {.width = 5,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 1},
                         {0, 2},
                         {0, 3},
                         {1, 0},
                         {1, 4},
                         {2, 0},
                         {2, 3},
                         {2, 4},
                         {3, 1},
                         {3, 2},
                         {3, 3},
                         {3, 4},
                         {4, 4}}}}}},
    {'R',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {0, 4},
                         {1, 0},
                         {1, 2},
                         {2, 1},
                         {2, 3},
                         {2, 4}}}}}},
    {'S',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1}, {1, 0}, {1, 2}, {1, 4}, {2, 0}, {2, 2}, {2, 4}, {3, 3}}}}}},
    {'T',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 0}, {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {2, 0}}}}}},
    {'U',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {1, 4},
                         {2, 4},
                         {3, 0},
                         {3, 1},
                         {3, 2},
                         {3, 3}}}}}},
    {'V',
     {.width = 3,
      .sections =
          {{.color = kWhite,
            .coords = {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {1, 4}, {2, 0}, {2, 1}, {2, 2}, {2, 3}}}}}},
    {'W',
     {.width = 5,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 1},
                         {0, 2},
                         {0, 3},
                         {1, 4},
                         {2, 2},
                         {2, 3},
                         {3, 4},
                         {4, 0},
                         {4, 1},
                         {4, 2},
                         {4, 3}}}}}},
    {'X',
     {.width = 3,
      .sections =
          {{.color = kWhite,
            .coords = {{0, 0}, {0, 1}, {0, 3}, {0, 4}, {1, 2}, {2, 0}, {2, 1}, {2, 3}, {2, 4}}}}}},
    {'Y',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 0}, {0, 1}, {1, 2}, {1, 3}, {1, 4}, {2, 0}, {2, 1}}}}}},
    {'Z',
     {.width = 3,
      .sections =
          {{.color = kWhite,
            .coords = {{0, 0}, {0, 3}, {0, 4}, {1, 0}, {1, 2}, {1, 4}, {2, 0}, {2, 1}, {2, 4}}}}}},

    {'0',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 1},
                         {0, 2},
                         {0, 3},
                         {1, 0},
                         {1, 2},
                         {1, 4},
                         {2, 0},
                         {2, 3},
                         {2, 4},
                         {3, 1},
                         {3, 2},
                         {3, 3}}}}}},
    {'1',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1}, {0, 4}, {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {2, 4}}}}}},
    {'2',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 1},
                         {0, 4},
                         {1, 0},
                         {1, 3},
                         {1, 4},
                         {2, 0},
                         {2, 2},
                         {2, 4},
                         {3, 1},
                         {3, 4}}}}}},
    {'3',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 0},
                         {0, 2},
                         {0, 4},
                         {1, 0},
                         {1, 2},
                         {1, 4},
                         {2, 0},
                         {2, 2},
                         {2, 4},
                         {3, 1},
                         {3, 3}}}}}},
    {'4',
     {.width = 4,
      .sections =
          {{.color = kWhite,
            .coords = {{0, 0}, {0, 1}, {1, 2}, {2, 2}, {3, 0}, {3, 1}, {3, 2}, {3, 3}, {3, 4}}}}}},
    {'5',
     {.width = 3,
      .sections =
          {{.color = kWhite,
            .coords = {{0, 0}, {0, 1}, {0, 2}, {0, 4}, {1, 0}, {1, 2}, {1, 4}, {2, 0}, {2, 3}}}}}},
    {'6',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1}, {0, 2}, {0, 3}, {1, 0}, {1, 2}, {1, 4}, {2, 0}, {2, 3}}}}}},
    {'7',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 0}, {1, 0}, {1, 3}, {1, 4}, {2, 0}, {2, 1}, {2, 2}}}}}},
    {'8',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1}, {0, 3}, {1, 0}, {1, 2}, {1, 4}, {2, 1}, {2, 3}}}}}},
    {'9',
     {.width = 3,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1}, {1, 0}, {1, 2}, {2, 1}, {2, 2}, {2, 3}, {2, 4}}}}}},

    {'$',
     {.width = 4,
      .sections = {{.color = kWhite,
                    .coords =
                        {{0, 2},
                         {1, 1},
                         {1, 3},
                         {1, 5},
                         {2, 0},
                         {2, 1},
                         {2, 3},
                         {2, 5},
                         {2, 6},
                         {3, 4}}}}}},
    {'!',
     {.width = 1, .sections = {{.color = kWhite, .coords = {{0, 0}, {0, 1}, {0, 2}, {0, 4}}}}}},
    {'%',
     {.width = 5,
      .sections = {{.color = kWhite,
                    .coords = {{0, 1}, {0, 4}, {1, 3}, {2, 2}, {3, 1}, {4, 0}, {4, 3}}}}}},
};

// todo: move to weather

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

std::unique_ptr<Page> makeTextPage();

std::unique_ptr<Page> makeWeather() {
  //

  return nullptr;
}

// todo: move to app/weather

struct WeatherPageImpl final : TextPage {
  const std::vector<SpritePlacement> &sprites() final { return _sprites; }

  void setText(const std::string &text) final {
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

//

struct TextPageImpl final : TextPage {
  const std::vector<SpritePlacement> &sprites() final { return _sprites; }

  void setText(const std::string &text) final {
    _sprites.clear();
    _sprites.reserve(text.size());

    for (auto x = 0, n = 0; n < text.size(); ++n) {
      auto it = kAlphaMap.find(text[n]);
      if (it == kAlphaMap.end()) {
        continue;
      }
      auto &glyph = it->second;
      _sprites.push_back({.pos = {x, 3}, .sprite = &glyph, .scale = {1, 2}});
      x += glyph.width + 1;
    }
  }

 private:
  std::vector<SpritePlacement> _sprites;
};

std::unique_ptr<TextPage> TextPage::create() { return std::make_unique<TextPageImpl>(); }

// make proper unit tests

int run() {
  std::string input;
  auto led = SpotiLED::create();
  auto page = TextPage::create();

  for (;;) {
    std::cin >> input;
    std::transform(input.begin(), input.end(), input.begin(), ::toupper);

    if (input.empty()) {
      return 0;
    }
    page->setText(input);
    StaticPresenter::create(*led, *page, 1);
  }

  return 0;
}

}  // namespace font
