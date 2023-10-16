#include "service.h"

#include <charconv>
#include <fstream>
#include <unordered_set>

#include "apps/settings/brightness_provider.h"
#include "authenticator_presenter.h"
#include "font/font.h"
#include "http/http.h"
#include "now_playing_presenter.h"
#include "present/presenter.h"
#include "util/storage/string_set.h"
#include "util/url/url.h"

namespace spotify {
namespace {

constexpr auto kTokensFilename = "spotify_token";

std::unordered_map<std::string, std::string> loadTokens() {
  std::unordered_map<std::string, std::string> now_playing;

  std::unordered_set<std::string> set;
  storage::loadSet(set, std::ifstream(kTokensFilename));
  for (auto token : set) {
    auto split = token.find('\n');
    now_playing[token.substr(0, split)] = token.substr(split + 1);
  }
  return now_playing;
}

void saveTokens(const std::unordered_map<std::string, std::string> &now_playing) {
  std::unordered_set<std::string> set;
  for (auto &[access_token, refresh_token] : now_playing) {
    set.insert(access_token + "\n" + refresh_token);
  }
  auto out = std::ofstream(kTokensFilename);
  storage::saveSet(set, out);
}

}  // namespace

SpotifyService::SpotifyService(async::Scheduler &main_scheduler,
                               http::Http &http,
                               SpotiLED &led,
                               present::PresenterQueue &presenter_queue,
                               settings::BrightnessProvider &brightness,
                               bool verbose)
    : _main_scheduler(main_scheduler),
      _http(http),
      _presenter_queue(presenter_queue),
      _brightness{brightness},
      _verbose(verbose) {
  for (auto &[access_token, refresh_token] : loadTokens()) {
    addNowPlaying(std::move(access_token), std::move(refresh_token));
  }
  std::cout << _now_playing_service.size() << " tokens loaded" << std::endl;
}

http::Response SpotifyService::handleRequest(http::Request req) {
  auto url = url::Url(req.url);

  if (auto it = req.headers.find("action"); it != req.headers.end()) {
    int show_login = 0;
    std::from_chars(it->second.data(), it->second.data() + it->second.size(), show_login);
    _show_login = show_login;
  } else {
    _show_login = !_show_login;
  }

  if (_show_login) {
    _presenter = AuthenticatorPresenter::create(
        _main_scheduler, _http, _presenter_queue, _brightness, _verbose,
        [this](auto access_token, auto refresh_token) {
          addNowPlaying(std::move(access_token), std::move(refresh_token));
          _pending_play = _now_playing_service.back().get();
          saveTokens();
        });
  } else {
    displaySomePlaying();
  }
  return 204;
}

void SpotifyService::onPlaying(const NowPlayingService &service, const NowPlaying &now_playing) {
  std::cout << "Spotify: started playing '" << now_playing.title << "'" << std::endl;
  if (&service == _pending_play || !_presenter) {
    _pending_play = nullptr;
    _playing = &service;
    _presenter = NowPlayingPresenter::create(_presenter_queue, _brightness, now_playing);
  }
}

void SpotifyService::onNewTrack(const NowPlayingService &, const NowPlaying &) {
  _presenter_queue.notify();
}

void SpotifyService::onStopped(const NowPlayingService &service) {
  std::cout << "Spotify: stopped playing" << std::endl;
  hideIfPlaying(service);
  displaySomePlaying();
}

void SpotifyService::onTokenUpdate(const NowPlayingService &) { saveTokens(); }

void SpotifyService::onLogout(const NowPlayingService &service) {
  hideIfPlaying(service);
  std::erase_if(_now_playing_service, [&](auto &s) { return s.get() == &service; });
  saveTokens();
  displaySomePlaying();
}

void SpotifyService::addNowPlaying(std::string access_token, std::string refresh_token) {
  _now_playing_service.push_back(NowPlayingService::create(
      _main_scheduler, _http, _verbose, std::move(access_token), std::move(refresh_token), *this));
}

void SpotifyService::displaySomePlaying() {
  auto get_some_playing = [this]() -> std::pair<const NowPlayingService *, const NowPlaying *> {
    for (auto &service : _now_playing_service) {
      if (auto *now_playing = service->getIfPlaying()) {
        return {service.get(), now_playing};
      }
    }
    return {nullptr, nullptr};
  };

  if (auto [service, now_playing] = get_some_playing(); service) {
    std::cout << "Spotify: playing '" << now_playing->title << "'" << std::endl;
    _playing = service;
    _presenter = NowPlayingPresenter::create(_presenter_queue, _brightness, *now_playing);
  } else {
    _presenter.reset();
  }
}

void SpotifyService::hideIfPlaying(const NowPlayingService &service) {
  if (&service == _playing) {
    _presenter.reset();
    _playing = nullptr;
  }
}

void SpotifyService::saveTokens() {
  std::unordered_map<std::string, std::string> tokens;
  for (auto &service : _now_playing_service) {
    tokens[service->accessToken()] = service->refreshToken();
  }
  ::spotify::saveTokens(tokens);
}

}  // namespace spotify
