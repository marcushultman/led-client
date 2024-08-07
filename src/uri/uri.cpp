#include "uri.h"

#include <charconv>

namespace uri {

inline auto split(std::string_view str, size_t pos, bool include, size_t npos) {
  auto [end, start] = pos != std::string_view::npos ? std::make_pair(pos, pos + (include ? 0 : 1))
                                                    : std::make_pair(npos, npos);
  return std::make_pair(str.substr(0, end), str.substr(start));
}

inline auto split(std::string_view str, char c, bool rev = false) {
  return split(str, str.find(c), false, rev ? str.size() : 0);
}

inline auto split(std::string_view str, std::string_view first_of, bool rev = false) {
  return split(str, str.find_first_of(first_of), true, rev ? str.size() : 0);
}

Uri::Uri(std::string_view url) {
  // scheme
  std::tie(scheme, url) = split(url, ':');

  // authority
  if (url.starts_with("//")) {
    std::tie(authority.userinfo, url) = split(url.substr(2), '@');
    std::tie(authority.host, url) = split(url, "/?#", true);
    std::string_view p;
    std::tie(authority.host, p) = split(authority.host, ':', true);
    if (!p.empty()) std::from_chars(p.begin(), p.end(), authority.port, 10);
  } else {
    authority.userinfo = url.substr(0, 0);
    authority.host = url.substr(0, 0);
  }

  // path
  std::tie(path.full, url) = split(url, "?#", true);

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

Uri::Path::Iterator::Iterator(std::string_view path) {
  std::tie(_segment, _tail) = split(path, '/', true);
}

Uri::Path::Iterator &Uri::Path::Iterator::operator++() {
  if (_tail.empty()) {
    _segment.reset();
  } else {
    std::tie(_segment, _tail) = split(_tail, '/', true);
  }
  return *this;
}

Uri::Path::Iterator Uri::Path::Iterator::operator++(int) {
  auto it = *this;
  ++(*this);
  return it;
}

}  // namespace uri
