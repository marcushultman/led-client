#include "url.h"

#include <charconv>

namespace url {

inline auto split(std::string_view str, size_t pos, bool include, bool rev) {
  auto [end, start] = pos != std::string_view::npos ? std::make_pair(pos, pos + (include ? 0 : 1))
                                                    : rev ? std::make_pair(str.size(), str.size())
                                                          : std::make_pair<size_t, size_t>(0, 0);
  return std::make_pair(str.substr(0, end), str.substr(start));
}

inline auto split(std::string_view str, char c, bool rev = false) {
  return split(str, str.find(c), false, rev);
}

inline auto split(std::string_view str, std::string_view first_of, bool rev = false) {
  return split(str, str.find_first_of(first_of), true, rev);
}

inline auto splitOrSkip(std::string_view str, std::string_view first_of) {
  auto [a, b] = split(str, first_of);
  return b.starts_with(first_of[0]) ? std::make_pair(a, b.substr(1))
                                    : std::make_pair(a.substr(0, 0), str);
}

Url::Url(std::string_view url) {
  // scheme
  std::tie(scheme, url) = splitOrSkip(url, ":/?#");

  // authority
  if (url.starts_with("//")) {
    std::tie(userinfo, url) = split(url.substr(2), '@');
    std::tie(host, url) = split(url, "/?#", true);
    std::string_view p;
    std::tie(host, p) = split(host, ':', true);
    if (!p.empty()) std::from_chars(p.begin(), p.end(), port, 10);
  } else {
    userinfo = url.substr(0, 0);
    host = url.substr(0, 0);
    port = 0;
  }

  // path
  std::tie(path.full, url) = split(url, "?#", true);
  path.segments.push_back(path.full.starts_with('/') ? path.full.substr(1) : path.full);

  for (std::string_view p;;) {
    std::tie(path.segments.back(), p) = split(path.segments.back(), '/', true);
    if (p.empty()) {
      break;
    }
    path.segments.push_back(p);
  }

  // query
  if (url.size() && url.front() == '?') {
    auto qs = url.substr(0, url.find('#'));
    url = url.substr(qs.size());
    while (qs.size()) {
      qs = qs.substr(1);
      auto eql = qs.find('=');
      auto end = std::min(qs.find('&'), qs.find('#'));
      if (eql < end) {
        q.emplace(qs.substr(0, eql), qs.substr((eql + 1), end - (eql + 1)));
      } else {
        q.emplace(qs.substr(0, end), "");
      }
      qs = qs.substr(end != std::string_view::npos ? end : qs.size());
    }
  }

  // fragment
  fragment = url.size() ? url.substr(1) : url;
}

}  // namespace url
