#include "util/storage/string_set.h"

#include <sstream>
#include <string>

int main(int argc, char *argv[]) {
  {
    std::unordered_set<std::string> set;
    storage::loadSet(set, std::istringstream(""));
    assert(set.empty());
  }
  {
    std::ostringstream data;
    storage::saveSet({"foo",
                      "barbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarb"
                      "arbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarba"
                      "rbarbarbarbarbarbarbarbarbar"},
                     data);
    std::unordered_set<std::string> set;
    storage::loadSet(set, std::istringstream(data.str()));
    assert(set.size() == 2);
    assert(set.count("foo") == 1);
    assert(set.count("barbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarb"
                     "arbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarbarba"
                     "rbarbarbarbarbarbarbarbarbar") == 1);
    assert(set.count("baz") == 0);
  }
}
