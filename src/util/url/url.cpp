#include "url.h"

namespace url {

Url::Url(std::string_view url) {
  scheme = url.substr(0, url.find_first_of("://"));
  url = url.substr(std::min(scheme.size() + 3, url.size()));
  host = url.substr(0, url.find_first_of("/"));
  url = url.substr(std::min(host.size() + 1, url.size()));
  {
    auto paths = url.substr(0, std::min(url.find_first_of('?'), url.find_first_of('#')));
    url = url.substr(paths.size());
    while (paths.size()) {
      path.push_back(paths.substr(0, paths.find_first_of('/')));
      paths = paths.substr(std::min(path.back().size() + 1, paths.size()));
    }
  }
  if (url.size() && url[0] == '?') {
    auto qs = url.substr(1, url.find_first_of('#'));
    url = url.substr(0, qs.size());
    while (qs.size()) {
      auto eql = qs.find_first_of('=');
      auto end = std::min(qs.find_first_of('&'), qs.find_first_of('#'));
      if (eql < end) {
        q.emplace(qs.substr(0, eql), qs.substr(eql + 1, end));
      } else {
        q.emplace(qs.substr(0, end), "");
      }
      qs = qs.substr(end + 1);
    }
  }
  if (url.size()) {
    fragment = url.substr(1);
  }
}

}  // namespace url
