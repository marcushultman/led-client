#include <stdio.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include "http/http.h"
#include "spotify_client.h"

int main(int argc, char *argv[]) {
  auto http = http::Http::create();
  if (!http) {
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

  auto main_thread = async::Thread::create("main");
  auto client = SpotifyClient::create(main_thread->scheduler(), *http, brightness, verbose);

  if (!client) {
    return 1;
  }

  main_thread->join();
  return 0;
}
