#include "spotify_client.h"
#include <algorithm>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  auto curl = curl_easy_init();
  if (!curl) {
    return 1;
  }
  auto jq = jq_init();
  if (!jq) {
    curl_easy_cleanup(curl);
    return 1;
  }

  auto verbose = false;
  uint8_t brightness = 1;

  for (auto i = 0; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.find("--verbose") == 0) {
      verbose = true;
    } else if (arg.find("--brightness") == 0) {
      // max 32 to avoid power brownout
      brightness = std::clamp(std::atoi(arg.data() + 13), 1, 32);
    }
  }
  return std::make_unique<SpotifyClient>(curl, jq, brightness, verbose)->run();
}
