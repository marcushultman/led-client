#pragma once

#include <memory>

#include "apps/settings/brightness_provider.h"
#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct SpotifyClient {
  virtual ~SpotifyClient() = default;

  static std::unique_ptr<SpotifyClient> create(async::Scheduler &main_scheduler,
                                               http::Http &,
                                               SpotiLED &led,
                                               present::PresenterQueue &presenter,
                                               settings::BrightnessProvider &brightness,
                                               bool verbose);
};
