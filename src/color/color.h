#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

namespace color::detail {

inline constexpr uint8_t mul(uint8_t lhs, double f) {
  return (lhs) ? std::max<uint8_t>(1, lhs * f) : 0;
}
inline constexpr uint8_t mul(uint8_t lhs, uint8_t rhs) {
  return (lhs && rhs) ? std::max(1, lhs * rhs) : 0;
}
inline constexpr uint8_t muld(uint8_t lhs, uint8_t rhs, uint8_t div) {
  return (lhs && rhs) ? std::max(1, lhs * rhs / div) : 0;
}

}  // namespace color::detail

struct Color : public std::array<uint8_t, 3> {
  using array::array;

  constexpr Color(uint8_t c) : array({c, c, c}) {}
  constexpr Color(uint8_t r, uint8_t g, uint8_t b) : array({r, g, b}) {}

  constexpr Color operator+(const Color &rhs) const {
    return Color(r() + rhs.r(), g() + rhs.g(), b() + rhs.b());
  }
  constexpr Color operator-(const Color &rhs) const {
    return Color(r() - rhs.r(), g() - rhs.g(), b() - rhs.b());
  }
  constexpr Color operator*(const Color &rhs) const {
    using color::detail::muld;
    return Color(muld(r(), rhs.r(), 255), muld(g(), rhs.g(), 255), muld(b(), rhs.b(), 255));
  }

  constexpr Color operator*(double f) const {
    using color::detail::mul;
    return Color(mul(r(), f), mul(g(), f), mul(b(), f));
  }
  constexpr Color operator*(uint8_t s) const { return *this * Color(s); }

  constexpr uint8_t &r() { return at(0); }
  constexpr uint8_t &g() { return at(1); }
  constexpr uint8_t &b() { return at(2); }
  constexpr uint8_t r() const { return at(0); }
  constexpr uint8_t g() const { return at(1); }
  constexpr uint8_t b() const { return at(2); }

  template <std::size_t I>
  constexpr std::tuple_element_t<I, Color> &get() {
    return at(I);
  }
};

constexpr Color operator*(double s, Color c) { return c * s; }
constexpr Color operator*(uint8_t s, Color c) { return c * s; }

namespace color {

constexpr auto kBlack = Color(0);
constexpr auto kWhite = Color(255);

inline uint8_t luminance(const Color &c) {
  return ((299 * c.r()) + (587 * c.g()) + (114 * c.b())) / 1000;
}

}  // namespace color

namespace std {

template <>
struct tuple_size<Color> : std::integral_constant<std::size_t, 3> {};

template <size_t Index>
struct tuple_element<Index, Color> : std::type_identity<uint8_t> {};

}  // namespace std
