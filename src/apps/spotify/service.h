#pragma once

#include <charconv>

#include "apps/settings/brightness_provider.h"
#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"
#include "spotify_client.h"
#include "util/url/url.h"

namespace spotify {

struct SpotifyService {
  SpotifyService(async::Scheduler &main_scheduler,
                 http::Http &http,
                 SpotiLED &led,
                 present::PresenterQueue &presenter_queue,
                 settings::BrightnessProvider &brightness,
                 bool verbose)
      : _main_scheduler(main_scheduler),
        _http(http),
        _led{led},
        _presenter_queue(presenter_queue),
        _brightness{brightness},
        _verbose(verbose),
        _now_playing_service{
            NowPlayingService::create(_main_scheduler, _http, _verbose, [this](auto &now_playing) {
              printf("Spotify: playing '%s'\n", now_playing.title.c_str());
              if (now_playing.access_token == _pending_token || !_presenter) {
                _pending_token.clear();
                _presenter =
                    NowPlayingPresenter::create(_presenter_queue, _brightness, now_playing);
              }
            })} {}

  http::Response handleRequest(http::Request req) {
    auto url = url::Url(req.url);

    if (auto it = req.headers.find("action"); it != req.headers.end()) {
      auto value = std::string_view(it->second);
      std::from_chars(value.begin(), value.end(), _mode);
    } else {
      _mode = (_mode + 1) % 2;
    }

    switch (_mode) {
      case 0:
        if (auto *now_playing = _now_playing_service->getSomePlaying()) {
          _presenter = NowPlayingPresenter::create(_presenter_queue, _brightness, *now_playing);
        } else {
          _presenter.reset();
        }
        break;
      case 1:
        _presenter = AuthenticatorPresenter::create(
            _main_scheduler, _http, _led, _presenter_queue, _brightness, _verbose,
            [this](auto access_token, auto refresh_token) {
              _pending_token = access_token;
              _now_playing_service->add(std::move(access_token), std::move(refresh_token));
            });
        break;
    }
    return 204;
  }

 private:
  async::Scheduler &_main_scheduler;
  http::Http &_http;
  SpotiLED &_led;
  present::PresenterQueue &_presenter_queue;
  settings::BrightnessProvider &_brightness;
  bool _verbose;

  std::unique_ptr<NowPlayingService> _now_playing_service;
  int _mode = 0;
  std::shared_ptr<void> _presenter;
  std::string _pending_token;
};

}  // namespace spotify
