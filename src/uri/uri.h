#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace uri {

struct Uri {
  std::string_view scheme;
  std::string_view userinfo;
  std::string_view host;
  int port = 0;
  struct Path {
    std::string_view full;
    std::vector<std::string_view> segments;
  } path;
  std::unordered_map<std::string, std::string_view> q;
  std::string_view fragment;

  Uri(std::string_view url);
  constexpr auto begin() const { return scheme.begin(); }
  constexpr auto end() const { return fragment.end(); }
};

}  // namespace uri
