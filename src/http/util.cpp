#include "util.h"

namespace http {

std::string_view trim(std::string_view value) {
  while (value.size() && value.front() == ' ') {
    value = value.substr(1);
  }
  return value.substr(0, std::min(value.find_first_of("\r"), value.find_first_of("\n")));
}

}  // namespace http