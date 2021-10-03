#include <vector>
#include <stdio.h>
#include "spotify_client.h"

int main() {
  auto curl = curl_easy_init();
  if (!curl) {
    return 1;
  }

  auto jq = jq_init();
  if (!jq) {
    curl_easy_cleanup(curl);
    return 1;
  }
  return std::make_unique<SpotifyClient>(curl, jq)->run();
}
