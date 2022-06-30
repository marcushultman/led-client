#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdio.h>
#include "spotify_client.h"

int main(int argc, char *argv[]) {
  auto curl = curl_easy_init();
  if (!curl) {
    return 1;
  }
  bool verbose = false;
  int brightness = 1;  // [0..255]

  for (auto i = 0; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.find("--verbose") == 0) {
      verbose = true;
    } else if (arg.find("--brightness") == 0) {
      brightness = std::clamp(std::atoi(std::string(arg.substr(13)).c_str()), 1, 255);
    }
  }

  auto jq = jq_init();
  if (!jq) {
    curl_easy_cleanup(curl);
    return 1;
  }
  return std::make_unique<SpotifyClient>(curl, jq, brightness, verbose)->run();
}
