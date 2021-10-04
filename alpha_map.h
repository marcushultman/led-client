#include <map>
#include <vector>

const auto kAlphaMap = std::map<char, std::vector<std::vector<int>>>{
    {'A',
     {{-5, -4, -3, -2, -1, 0, 1, 2}, {-1, 0, 3, 4}, {-1, 0, 3, 4}, {-5, -4, -3, -2, -1, 0, 1, 2}}},
    {'B',
     {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4},
      {-5, -4, -1, 0, 3, 4},
      {-5, -4, -1, 0, 3, 4},
      {-3, -2, 1, 2}}},
    {'C', {{-3, -2, -1, 0, 1, 2}, {-5, -4, 3, 4}, {-5, -4, 3, 4}, {-5, -4, 3, 4}}},
    {'D',
     {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-5, -4, 3, 4}, {-5, -4, 3, 4}, {-3, -2, -1, 0, 1, 2}}},
    {'E',
     {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4},
      {-5, -4, -1, 0, 3, 4},
      {-5, -4, -1, 0, 3, 4},
      {-5, -4, 3, 4}}},
    {'F', {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-1, 0, 3, 4}, {-1, 0, 3, 4}, {3, 4}}},
    {'G', {{-3, -2, -1, 0, 1, 2}, {-5, -4, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-3, -2, -1, 0, 3, 4}}},
    {'H',
     {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-1, 0}, {-1, 0}, {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}}},
    {'I', {{-5, -4, 3, 4}, {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-5, -4, 3, 4}}},
    {'J', {{-3, -2}, {-5, -4}, {-5, -4}, {-3, -2, -1, 0, 1, 2, 3, 4}}},
    {'K', {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-1, 0}, {-3, -2, 1, 2}, {-5, -4, 3, 4}}},
    {'L', {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-5, -4}, {-5, -4}}},
    {'M',
     {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4},
      {1, 2},
      {-1, 0},
      {1, 2},
      {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}}},
    {'N',
     {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {1, 2}, {-1, 0}, {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}}},
    {'O', {{-3, -2, -1, 0, 1, 2}, {-5, -4, 3, 4}, {-5, -4, 3, 4}, {-3, -2, -1, 0, 1, 2}}},
    {'P', {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-1, 0, 3, 4}, {-1, 0, 3, 4}, {1, 2}}},
    {'Q',
     {{-3, -2, -1, 0, 1, 2},
      {-5, -4, 3, 4},
      {-5, -4, -3, -2, 3, 4},
      {-5, -4, -3, -2, -1, 0, 1, 2}}},
    {'R', {{-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-1, 0, 3, 4}, {-3, -2, -1, 3, 4}, {-5, -4, 1, 2}}},
    {'S', {{-5, -4, 1, 2}, {-5, -4, -1, 0, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-3, -2, 3, 4}}},
    {'T', {{3, 4}, {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {3, 4}}},
    {'U', {{-3, -2, -1, 0, 1, 2, 3, 4}, {-5, -4}, {-5, -4}, {-3, -2, -1, 0, 1, 2, 3, 4}}},
    {'V', {{-3, -2, -1, 0, 1, 2, 3, 4}, {-5, -4}, {-3, -2, -1, 0, 1, 2, 3, 4}}},
    {'W', {{-3, -2, -1, 0, 1, 2, 3, 4}, {-5, -4}, {-3, -2}, {-5, -4}, {-3, -2, -1, 0, 1, 2, 3, 4}}},
    {'X', {{-5, -4, -3, -2, 1, 2, 3, 4}, {-1, 0}, {-5, -4, -3, -2, 1, 2, 3, 4}}},
    {'Y', {{0, 1, 2, 3, 4}, {-5, -4, -3, -2, -1}, {0, 1, 2, 3, 4}}},
    {'Z', {{-5, -4, 3, 4}, {-5, -4, -3, -2, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-5, -4, 1, 2, 3, 4}}},

    {'1', {{-5, -4, 1, 2}, {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}, {-5, -4}}},
    {'2', {{-5, -4, 1, 2}, {-5, -4, -3, -2, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-5, -4, 1, 2}}},
    {'3', {{-5, -4, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-3, -2, 1, 2}}},
    {'4', {{-1, 0, 1, 2, 3, 4}, {-1, 0}, {-1, 0}, {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4}}},
    {'5',
     {{-5, -4, -1, 0, 1, 2, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-3, -2, 3, 4}}},
    {'6', {{-3, -2, -1, 0, 1, 2}, {-5, -4, -1, 0, 3, 4}, {-5, -4, -1, 0}, {-3, -2, -1, 0}}},
    {'7', {{3, 4}, {-5, -4, -3, -2, 3, 4}, {-1, 0, 3, 4}, {1, 2, 3, 4}}},
    {'8', {{-3, -2, 1, 2}, {-5, -4, -1, 0, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-3, -2, 1, 2}}},
    {'9', {{-5, -4, 1, 2}, {-5, -4, -1, 0, 3, 4}, {-5, -4, -1, 0, 3, 4}, {-3, -2, -1, 0, 1, 2}}},
};
