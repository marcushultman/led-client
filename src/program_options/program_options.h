#pragma once

#include <string>

namespace program_options {

struct Options {
  bool verbose = false;
  std::string base_url;
};

Options parseOptions(int argc, char *argv[]);

}  // namespace program_options
