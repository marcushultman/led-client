#include "uri.h"

#include <algorithm>
#include <charconv>
#include <tuple>

namespace uri {

using Split = std::pair<std::string_view, std::string_view>;

inline auto split(std::string_view str, size_t pos, int start = 1, int end = 0) {
  return pos != std::string_view::npos
             ? std::make_optional<Split>(str.substr(0, pos + end), str.substr(pos + start))
             : std::nullopt;
}

inline auto split(std::string_view str, char c) {
  return split(str, str.find(c)).value_or(Split{str.substr(0, 0), str});
}

inline auto rsplit(std::string_view str, char c) {
  return split(str, str.find(c)).value_or(Split{str, str.substr(str.size())});
}

inline auto split(std::string_view str, std::string_view first_of, int start = 0) {
  return split(str, str.find_first_of(first_of), start).value_or(Split{str.substr(0, 0), str});
}

inline auto rsplit(std::string_view str, std::string_view first_of, int start = 0) {
  return split(str, str.find_first_of(first_of), start)
      .value_or(Split{str, str.substr(str.size())});
}

Uri::Uri(std::string_view url) {
  std::tie(scheme, url) = split(url, ':');

  if (url.starts_with("//")) {
    auto &[full, userinfo, host, port] = authority;
    std::tie(full, url) = rsplit(url.substr(2), "/?#");
    std::tie(userinfo, host) = split(full, '@');
    std::string_view port_str;
    std::tie(host, port_str) = host.starts_with('[')
                                   ? split(host, host.find("]:"), 2, 1).value_or<Split>({host, {}})
                                   : rsplit(host, ':');
    if (!port_str.empty()) {
      std::from_chars(port_str.begin(), port_str.end(), port, 10);
    }
  } else {
    authority.full = url.substr(0, 0);
    authority.userinfo = url.substr(0, 0);
    authority.host = url.substr(0, 0);
  }

  std::tie(path.full, url) = rsplit(url, "?#");

  if (url.starts_with('?')) {
    std::tie(query.full, url) = rsplit(url.substr(1), "#");
  } else {
    query.full = url.substr(0, 0);
  }

  fragment = !url.empty() ? url.substr(1) : url.substr(0, 0);
}

Uri::Path::Iterator::Iterator(std::string_view path) {
  std::tie(_segment, _tail) = rsplit(path, '/');
}

Uri::Path::Iterator::value_type Uri::Path::Iterator::operator*() const { return *_segment; }

Uri::Path::Iterator &Uri::Path::Iterator::operator++() {
  if (_tail.empty()) {
    _segment.reset();
  } else {
    std::tie(_segment, _tail) = rsplit(_tail, '/');
  }
  return *this;
}

Uri::Path::Iterator Uri::Path::Iterator::operator++(int) {
  auto it = *this;
  ++(*this);
  return it;
}

Uri::Query::Iterator::Iterator(std::string_view query) {
  std::tie(_segment, _tail) = rsplit(query, "&?", 1);
}

Uri::Query::Iterator::value_type Uri::Query::Iterator::operator*() const {
  return rsplit(*_segment, '=');
}

Uri::Query::Iterator &Uri::Query::Iterator::operator++() {
  if (_tail.empty()) {
    _segment.reset();
  } else {
    std::tie(_segment, _tail) = rsplit(_tail, "&?", 1);
  }
  return *this;
}

Uri::Query::Iterator Uri::Query::Iterator::operator++(int) {
  auto it = *this;
  ++(*this);
  return it;
}

bool Uri::Query::has(std::string_view key) const {
  return std::count_if(begin(), end(), [&](auto kv) { return kv.first == key; });
}

std::string_view Uri::Query::find(std::string_view key) const {
  auto it = std::find_if(begin(), end(), [&](auto kv) { return kv.first == key; });
  return it != end() ? (*it).second : "";
}

}  // namespace uri
