#pragma once

#include <iterator>
#include <optional>
#include <string_view>

namespace uri {

struct Uri {
  struct Authority {
    std::string_view full;
    std::string_view userinfo;
    std::string_view host;
    int port = 0;
  };
  struct Path {
    struct Iterator {
      using iterator_concept = std::forward_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = std::string_view;

      constexpr Iterator() = default;

      value_type operator*() const;
      Iterator &operator++();
      Iterator operator++(int);
      friend bool operator==(const Iterator &lhs, const Iterator &rhs) = default;

     private:
      friend struct Path;
      Iterator(std::string_view path);

      std::optional<std::string_view> _segment;
      std::string_view _tail;
    };
    Path() = default;
    Path(std::string_view full) : full{full} {}

    std::string_view full;

    auto begin() const { return Iterator{full.starts_with('/') ? full.substr(1) : full}; }
    auto end() const { return Iterator{}; }
    auto front() const { return *begin(); }
    auto back() const {
      for (auto it = begin(), b = it;; b = it) {
        if (++it == end()) return *b;
      }
    }
  };
  struct Query {
    struct Iterator {
      using iterator_concept = std::forward_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = std::pair<std::string_view, std::string_view>;

      constexpr Iterator() = default;

      value_type operator*() const;
      Iterator &operator++();
      Iterator operator++(int);
      friend bool operator==(const Iterator &lhs, const Iterator &rhs) = default;

     private:
      friend struct Query;
      Iterator(std::string_view path);

      std::optional<std::string_view> _segment;
      std::string_view _tail;
    };
    Query() = default;
    Query(std::string_view full) : full{full} {}

    std::string_view full;

    auto begin() const { return Iterator{full}; }
    auto end() const { return Iterator{}; }
    bool has(std::string_view key) const;
    std::string_view find(std::string_view key) const;
  };

  std::string_view scheme;
  Authority authority;
  Path path;
  Query query;
  std::string_view fragment;

  Uri(std::string_view url);
  constexpr auto begin() const { return scheme.begin(); }
  constexpr auto end() const { return fragment.end(); }
};

static_assert(std::forward_iterator<Uri::Path::Iterator>);
static_assert(std::forward_iterator<Uri::Query::Iterator>);

}  // namespace uri
