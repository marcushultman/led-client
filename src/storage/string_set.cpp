#include "string_set.h"

namespace storage {

void loadSet(std::unordered_set<std::string> &set, std::istream &&stream) {
  std::string scratch;
  while (stream.good()) {
    auto size = 0;
    std::istream::int_type c;
    auto shift = 0;
    do {
      c = stream.get();
      if (c == EOF) {
        return;
      }
      size += (c & 0x7f) << shift;
      shift += 7;
    } while (c & 0x80);
    scratch.resize(size);
    stream.read(scratch.data(), size);
    set.insert(scratch);
  }
}

void saveSet(const std::unordered_set<std::string> &set, std::ostream &stream) {
  for (auto &value : set) {
    auto size = value.size();
    while (size > 0x7f) {
      stream.put(0x80 | (size & 0x7f));
      size >>= 7;
    }
    stream.put(size);
    stream.write(value.data(), value.size());
  }
}

}  // namespace storage
