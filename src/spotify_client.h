#pragma once

#include <memory>

#include "async/scheduler.h"
#include "http/http.h"
#include "spotiled.h"

struct SpotifyClient {
  virtual ~SpotifyClient() = default;

  static std::unique_ptr<SpotifyClient> create(async::Scheduler &main_scheduler,
                                               http::Http &,
                                               SpotiLED &led,
                                               BrightnessProvider &brightness,
                                               bool verbose);
};
