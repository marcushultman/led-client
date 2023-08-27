#pragma once

#include <curl/curl.h>

#include <memory>

extern "C" {
#include <jq.h>
}

struct SpotifyClient {
  virtual ~SpotifyClient() = default;
  virtual int run() = 0;

  static std::unique_ptr<SpotifyClient> create(CURL *, jq_state *, uint8_t brightness,
                                               bool verbose);
};
