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

  http::Lifetime request;
  async::Lifetime work;
};

std::chrono::milliseconds requestBackoff(size_t num_request);

struct NowPlayingService {
  using OnPlaying = std::function<void(const NowPlayingService &, const NowPlaying &)>;
  using OnTokenUpdate = std::function<void(const NowPlayingService &)>;
  using OnLogout = std::function<void(const NowPlayingService &)>;

  virtual ~NowPlayingService() = default;
  virtual const NowPlaying *getIfPlaying() const = 0;
  virtual const std::string &accessToken() const = 0;
  virtual const std::string &refreshToken() const = 0;

  static std::unique_ptr<NowPlayingService> create(async::Scheduler &main_scheduler,
                                                   http::Http &,
                                                   bool verbose,
                                                   std::string access_token,
                                                   std::string refresh_token,
                                                   OnPlaying,
                                                   OnTokenUpdate,
                                                   OnLogout);
};

struct AuthenticatorPresenter {
  using AccessTokenCallback =
      std::function<void(std::string access_token, std::string refresh_token)>;
  virtual ~AuthenticatorPresenter() = default;

  // API?

  static std::unique_ptr<AuthenticatorPresenter> create(async::Scheduler &main_scheduler,
                                                        http::Http &,
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
