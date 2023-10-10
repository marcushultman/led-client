#include "service.h"

#include <charconv>
#include <fstream>
#include <unordered_set>

#include "apps/settings/brightness_provider.h"
#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"
#include "spotify_client.h"
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
  printf("%lu tokens loaded\n", _now_playing_service.size());
}

http::Response SpotifyService::handleRequest(http::Request req) {
  auto url = url::Url(req.url);

  if (auto it = req.headers.find("action"); it != req.headers.end()) {
    auto value = std::string_view(it->second);
    std::from_chars(value.begin(), value.end(), _mode);
  } else {
    _mode = (_mode + 1) % 2;
  }

  switch (_mode) {
    case 0:
      if (auto *now_playing = getSomePlaying()) {
        _presenter = NowPlayingPresenter::create(_presenter_queue, _brightness, *now_playing);
      } else {
        _presenter.reset();
      }
      break;
    case 1:
      _presenter = AuthenticatorPresenter::create(
          _main_scheduler, _http, _presenter_queue, _brightness, _verbose,
          [this](auto access_token, auto refresh_token) {
            addNowPlaying(std::move(access_token), std::move(refresh_token));
            _pending_play = _now_playing_service.back().get();
            saveTokens();
          });
      break;
  }
  return 204;
}

void SpotifyService::addNowPlaying(std::string access_token, std::string refresh_token) {
  _now_playing_service.push_back(NowPlayingService::create(
      _main_scheduler, _http, _verbose, std::move(access_token), std::move(refresh_token),
      [this](auto &service, auto &now_playing) {
        printf("Spotify: playing '%s'\n", now_playing.title.c_str());
        if (&service == _pending_play || !_presenter) {
          _pending_play = nullptr;
          _presenter = NowPlayingPresenter::create(_presenter_queue, _brightness, now_playing);
        }
      },
      [this](auto &) { saveTokens(); },
      [this](auto &service) {
        std::erase_if(_now_playing_service, [&](auto &s) { return s.get() == &service; });
        saveTokens();
      }));
}

const NowPlaying *SpotifyService::getSomePlaying() const {
  for (auto &service : _now_playing_service) {
    if (auto *now_playing = service->getIfPlaying()) {
      return now_playing;
    }
  }
  return nullptr;
}

void SpotifyService::saveTokens() {
  std::unordered_map<std::string, std::string> tokens;
  for (auto &service : _now_playing_service) {
    tokens[service->accessToken()] = service->refreshToken();
  }
  ::spotify::saveTokens(tokens);
}

}  // namespace spotify
