#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <string>

#include "async/scheduler.h"
#include "http/http.h"

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
  struct Callbacks {
    virtual ~Callbacks() = default;
    virtual void onPlaying(const NowPlayingService &, const NowPlaying &) = 0;
    virtual void onNewTrack(const NowPlayingService &, const NowPlaying &) = 0;
    virtual void onStopped(const NowPlayingService &) = 0;
    virtual void onTokenUpdate(const NowPlayingService &) = 0;
    virtual void onLogout(const NowPlayingService &) = 0;
  };

  virtual ~NowPlayingService() = default;
  virtual const NowPlaying *getIfPlaying() const = 0;
  virtual const std::string &accessToken() const = 0;
  virtual const std::string &refreshToken() const = 0;

  static std::unique_ptr<NowPlayingService> create(async::Scheduler &main_scheduler,
                                                   http::Http &,
                                                   bool verbose,
                                                   std::string access_token,
                                                   std::string refresh_token,
                                                   Callbacks &);
};

}  // namespace spotify
