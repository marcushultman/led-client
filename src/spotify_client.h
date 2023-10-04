#pragma once

#include <memory>

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
#include "spotiled.h"

struct SpotifyClient {
  virtual ~SpotifyClient() = default;

  static std::unique_ptr<SpotifyClient> create(async::Scheduler &main_scheduler,
                                               http::Http &,
                                               SpotiLED &led,
                                               present::PresenterQueue &presenter,
                                               settings::BrightnessProvider &brightness,
                                               bool verbose);
};
