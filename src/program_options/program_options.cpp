#include "program_options.h"

namespace program_options {

Options parseOptions(int argc, char *argv[]) {
  Options opts;
  for (auto i = 0; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.find("--verbose") == 0) {
      opts.verbose = true;
    } else if (arg.find("--base-url") == 0) {
      opts.base_url = arg.substr(11);
    } else if (arg.find("--ikea") == 0) {
      opts.ikea = true;
    }
  }
  return opts;
}

}  // namespace program_options
