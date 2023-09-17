#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

struct Color : public std::array<uint8_t, 3> {
  using array::array;

  constexpr Color(uint8_t c) : array({c, c, c}) {}
  constexpr Color(uint8_t r, uint8_t g, uint8_t b) : array({r, g, b}) {}

  constexpr Color operator*(const Color &rhs) const {
    return {static_cast<uint8_t>(static_cast<int>(at(0)) * rhs[0] / 255),
            static_cast<uint8_t>(static_cast<int>(at(1)) * rhs[1] / 255),
            static_cast<uint8_t>(static_cast<int>(at(2)) * rhs[2] / 255)};
  }

  constexpr Color operator*(uint8_t s) const {
    return {static_cast<uint8_t>(static_cast<int>(at(0)) * s / 255),
            static_cast<uint8_t>(static_cast<int>(at(1)) * s / 255),
            static_cast<uint8_t>(static_cast<int>(at(2)) * s / 255)};
  }

  template <std::size_t I>
  constexpr std::tuple_element_t<I, Color> &get() {
    return at(I);
  }
};

constexpr auto kBlack = Color(0);
constexpr auto kWhite = Color(255);

namespace std {

template <>
struct tuple_size<Color> : std::integral_constant<std::size_t, 3> {};

template <size_t Index>
struct tuple_element<Index, Color> : std::type_identity<uint8_t> {};

}  // namespace std
