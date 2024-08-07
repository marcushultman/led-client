#include <uri/uri.h>

#include <cassert>
#include <vector>

template <typename C>
bool operator==(const uri::Uri::Path &path, const C &expected) {
  return std::equal(path.begin(), path.end(), expected.begin());
}

int main(int argc, char *argv[]) {
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("");
    assert(path == std::vector{""});
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("/");
    assert(path == std::vector{""});
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("//");
    assert(path == std::vector{""});
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("://///");
    assert(path == std::vector({"", ""}));
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("file:/");
    assert(path == std::vector{""});
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("/foo/bar/baz");
    assert(path == std::vector({"foo", "bar", "baz"}));
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("file:foo/bar/baz");
    assert(path == std::vector({"foo", "bar", "baz"}));
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("/foo/bar/baz/?");
    assert(path == std::vector({"foo", "bar", "baz"}));
  }
  {
    const auto &[scheme, _, path, q, f] = uri::Uri("/foo/bar/baz/#");
    assert(path == std::vector({"foo", "bar", "baz"}));
  }
}
