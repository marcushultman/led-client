#pragma once

#include <utility>
#include <vector>

#include "http/http.h"
#include "now_playing_service.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

extern "C" {
#include <jq.h>
}

namespace spotify {

struct AuthenticatorPresenter;

class SpotifyService : NowPlayingService::Callbacks {
 public:
  SpotifyService(async::Scheduler &main_scheduler,
                 http::Http &http,
                 spotiled::Renderer &,
                 present::PresenterQueue &presenter_queue,
                 bool verbose);

  http::Response handleRequest(http::Request req);

  bool isAuthenticating() const;
  jv getTokens() const;

 private:
  //  NowPlayingService::Callbacks
  void onPlaying(const NowPlayingService &, const NowPlaying &) final;
  void onNewTrack(const NowPlayingService &, const NowPlaying &) final;
  void onStopped(const NowPlayingService &) final;
  void onTokenUpdate(const NowPlayingService &) final;
  void onLogout(const NowPlayingService &) final;

  void addNowPlaying(std::string access_token, std::string refresh_token);

  void displaySomePlaying();
  void hideIfPlaying(const NowPlayingService &);

  void saveTokens();

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  present::PresenterQueue &_presenter_queue;
  bool _verbose;

  std::vector<std::unique_ptr<NowPlayingService>> _now_playing_service;
  AuthenticatorPresenter *_authenticator = nullptr;
  spotiled::Renderer &_renderer;
  std::shared_ptr<void> _presenter;
  const NowPlayingService *_pending_play = nullptr;
  const NowPlayingService *_playing = nullptr;
};

}  // namespace spotify
