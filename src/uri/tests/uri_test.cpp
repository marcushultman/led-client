#include <uri/uri.h>

#include <cassert>
#include <iostream>

using namespace std::string_view_literals;

template <typename C, typename = C::value_type>
bool operator==(const uri::Uri::Path &path, const C &expected) {
  return std::equal(path.begin(), path.end(), expected.begin());
}

template <typename C, typename = C::value_type::first_type>
bool operator==(const uri::Uri::Query &query, const C &expected) {
  return std::equal(query.begin(), query.end(), expected.begin());
}

int main(int argc, char *argv[]) {
  {
    const auto &[scheme, authority, path, query, fragment] = uri::Uri(
        "https://john.doe@www.example.com:1234/forum/questions/?tag=networking&order=newest#top");
    assert(scheme == "https");
    assert(authority.full == "john.doe@www.example.com:1234");
    assert(authority.userinfo == "john.doe");
    assert(authority.host == "www.example.com");
    assert(authority.port == 1234);
    assert(path.full == "/forum/questions/");
    assert(path == std::vector({"forum", "questions"}));
    assert(query.full == "tag=networking&order=newest");
    assert(query == std::vector({std::make_pair("tag"sv, "networking"sv),
                                 std::make_pair("order"sv, "newest"sv)}));
    assert(query.find("tag") == "networking");
    assert(query.has("order"));
    assert(!query.has("nonexistent"));
    assert(fragment == "top");
  }
  {
    const auto &[scheme, authority, path, query, fragment] = uri::Uri(
        "https://john.doe@www.example.com:1234/forum/questions/"
        "?tag=networking&order=newest#:~:text=whatever");
    assert(scheme == "https");
    assert(authority.full == "john.doe@www.example.com:1234");
    assert(authority.userinfo == "john.doe");
    assert(authority.host == "www.example.com");
    assert(authority.port == 1234);
    assert(path.full == "/forum/questions/");
    assert(path == std::vector({"forum", "questions"}));
    assert(fragment == ":~:text=whatever");
  };
  {
    const auto &[scheme, authority, path, query, fragment] =
        uri::Uri("ldap://[2001:db8::7]/c=GB?objectClass?one");
    assert(scheme == "ldap");
    assert(authority.full == "[2001:db8::7]");
    assert(authority.userinfo.empty());
    assert(authority.host == "[2001:db8::7]");
    assert(authority.port == 0);
    assert(path.full == "/c=GB");
    assert(path == std::vector({"c=GB"}));
    assert(query.full == "objectClass?one");
    assert(query.has("objectClass"));
    assert(query.has("one"));
    assert(fragment.empty());
  }
  {
    const auto &[scheme, authority, path, query, fragment] =
        uri::Uri("mailto:John.Doe@example.com");
    assert(scheme == "mailto");
    assert(path.full == "John.Doe@example.com");
    assert(path == std::vector({"John.Doe@example.com"}));
    assert(query.full.empty());
    assert(fragment.empty());
  }
  {
    const auto &[scheme, authority, path, query, fragment] =
        uri::Uri("news:comp.infosystems.www.servers.unix");
    assert(scheme == "news");
    assert(path.full == "comp.infosystems.www.servers.unix");
    assert(path == std::vector({"comp.infosystems.www.servers.unix"}));
    assert(query.full.empty());
    assert(fragment.empty());
  }
  {
    const auto &[scheme, authority, path, query, fragment] = uri::Uri("tel:+1-816-555-1212");
    assert(scheme == "tel");
    assert(path.full == "+1-816-555-1212");
    assert(path == std::vector({"+1-816-555-1212"}));
    assert(query.full.empty());
    assert(fragment.empty());
  }
  {
    const auto &[scheme, authority, path, query, fragment] = uri::Uri("telnet://192.0.2.16:80/");
    assert(scheme == "telnet");
    assert(authority.full == "192.0.2.16:80");
    assert(authority.host == "192.0.2.16");
    assert(authority.port == 80);
    assert(path.full == "/");
    assert(path == std::vector({""}));
    assert(query.full.empty());
    assert(fragment.empty());
  }
  {
    const auto &[scheme, authority, path, query, fragment] =
        uri::Uri("urn:oasis:names:specification:docbook:dtd:xml:4.1.2");
    assert(scheme == "urn");
    assert(authority.full.empty());
    assert(authority.host.empty());
    assert(authority.port == 0);
    assert(path.full == "oasis:names:specification:docbook:dtd:xml:4.1.2");
    assert(path == std::vector({"oasis:names:specification:docbook:dtd:xml:4.1.2"}));
    assert(query.full.empty());
    assert(fragment.empty());
  }

  {
    const auto &[scheme, authority, path, query, fragment] = uri::Uri("foo:#bar");
    assert(scheme == "foo");
    assert(authority.full.empty());
    assert(path.full.empty());
    assert(query.full.empty());
    assert(fragment == "bar");
  }
  std::cout << "OK" << std::endl;
}
