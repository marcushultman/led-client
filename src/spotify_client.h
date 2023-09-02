#pragma once

#include <memory>

#include "async/scheduler.h"
#include "http/http.h"

struct SpotifyClient {
  virtual ~SpotifyClient() = default;

  static std::unique_ptr<SpotifyClient> create(async::Scheduler &main_scheduler,
                                               http::Http &,
                                               uint8_t brightness,
                                               bool verbose);
};
