#include <string>
#include <vector>
#include <stdio.h>
#include "spotify_client.h"

int main(int argc, char *argv[]) {
  auto curl = curl_easy_init();
  if (!curl) {
    return 1;
  }
  auto verbose = argc > 0 && std::string_view(argv[0]) == "--verbose";

  auto jq = jq_init();
  if (!jq) {
    curl_easy_cleanup(curl);
    return 1;
  }
  return std::make_unique<SpotifyClient>(curl, jq, verbose)->run();
}
