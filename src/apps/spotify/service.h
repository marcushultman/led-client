#pragma once

#include "apps/settings/brightness_provider.h"
#include "http/http.h"
#include "present/presenter.h"
#include "spotify_client.h"

namespace spotify {

struct SpotifyService {
  SpotifyService(async::Scheduler &main_scheduler,
                 http::Http &http,
                 SpotiLED &led,
                 present::PresenterQueue &presenter_queue,
                 settings::BrightnessProvider &brightness,
                 bool verbose);

  http::Response handleRequest(http::Request req);

 private:
  void addNowPlaying(std::string access_token, std::string refresh_token);
  const NowPlaying *getSomePlaying() const;

  void saveTokens();

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  present::PresenterQueue &_presenter_queue;
  settings::BrightnessProvider &_brightness;
  bool _verbose;

  std::vector<std::unique_ptr<NowPlayingService>> _now_playing_service;
  int _mode = 0;
  std::shared_ptr<void> _presenter;
  const NowPlayingService *_pending_play = nullptr;
};

}  // namespace spotify
