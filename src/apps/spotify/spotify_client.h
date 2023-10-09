#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "apps/settings/brightness_provider.h"
#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

namespace spotify {

struct NowPlaying {
  int status = 0;
  std::string access_token;
  std::string refresh_token;
  std::chrono::system_clock::time_point requested_at = {};
  size_t num_request = 0;

  std::string track_id;
  std::string context_href;
  std::string context;
  std::string title;
  std::string artist;
  std::string image;
  std::string uri;
  std::chrono::milliseconds progress = {};
  std::chrono::milliseconds duration = {};
  std::array<uint8_t, 23> lengths0, lengths1;
};

std::chrono::milliseconds requestBackoff(size_t num_request);

// todo: split this in 1 service instance per user
struct NowPlayingService {
  using OnPlaying = std::function<void(const NowPlaying &)>;
  virtual ~NowPlayingService() = default;
  virtual void add(std::string access_token, std::string refresh_token) = 0;
  virtual const NowPlaying *getSomePlaying() const = 0;

  static std::unique_ptr<NowPlayingService> create(async::Scheduler &main_scheduler,
                                                   http::Http &,
                                                   bool verbose,
                                                   OnPlaying);
};

struct AuthenticatorPresenter {
  using AccessTokenCallback =
      std::function<void(std::string access_token, std::string refresh_token)>;
  virtual ~AuthenticatorPresenter() = default;

  // API?

  static std::unique_ptr<AuthenticatorPresenter> create(async::Scheduler &main_scheduler,
                                                        http::Http &,
                                                        SpotiLED &led,
                                                        present::PresenterQueue &presenter,
                                                        settings::BrightnessProvider &brightness,
                                                        bool verbose,
                                                        AccessTokenCallback);
};

struct NowPlayingPresenter {
  virtual ~NowPlayingPresenter() = default;
  static std::unique_ptr<NowPlayingPresenter> create(present::PresenterQueue &,
                                                     settings::BrightnessProvider &,
                                                     const NowPlaying &);
};

}  // namespace spotify
