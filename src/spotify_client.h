#pragma once

#include <memory>

#include "http/http.h"

extern "C" {
#include <jq.h>
}

struct SpotifyClient {
  virtual ~SpotifyClient() = default;
  virtual int run() = 0;

  static std::unique_ptr<SpotifyClient> create(http::Http &,
                                               jq_state *,
                                               uint8_t brightness,
                                               bool verbose);
};
