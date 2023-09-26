#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace url {

struct Url {
  std::string_view scheme;
  std::string_view host;
  std::vector<std::string_view> path;
  std::unordered_map<std::string, std::string_view> q;
  std::string_view fragment;

  Url(std::string_view url);
};

}  // namespace url
