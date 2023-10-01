#pragma once

#include <charconv>

#include "font/font.h"
#include "http/http.h"
#include "spotify_client.h"
#include "spotiled.h"
#include "util/presenter.h"
#include "util/url.h"

namespace spotify {

const auto kDefaultHttpResponse = http::Response(
    "<html><body><form action=\"/text\" method=\"post\"><input name=\"text\" "
    "placeholder=\"\"><input type=\"submit\"></form></body></html>");

struct SpotifyService {
  SpotifyService(async::Scheduler &main_scheduler,
                 http::Http &http,
                 SpotiLED &led,
                 presenter::PresenterQueue &presenter,
                 BrightnessProvider &brightness,
                 bool verbose)
      : _main_scheduler(main_scheduler),
        _http(http),
        _led{led},
        _presenter(presenter),
        _brightness{brightness},
        _verbose(verbose) {}

  http::Response handleRequest(http::Request req) {
    auto url = url::Url(req.url);
    if (url.path.empty() || url.path.back() != "mode") {
      return kDefaultHttpResponse;
    }

    if (auto it = req.headers.find("action"); it != req.headers.end()) {
      auto value = std::string_view(it->second);
      std::from_chars(value.begin(), value.end(), _mode);
    } else {
      _mode = (_mode + 1) % 2;
    }

    switch (_mode) {
      case 0:
        _runner.reset();
        return kDefaultHttpResponse;
      case 1:
        _runner =
            SpotifyClient::create(_main_scheduler, _http, _led, _presenter, _brightness, _verbose);
        return kDefaultHttpResponse;
    }
    return 204;
  }

 private:
  async::Scheduler &_main_scheduler;
  http::Http &_http;
  SpotiLED &_led;
  presenter::PresenterQueue &_presenter;
  BrightnessProvider &_brightness;
  bool _verbose;

  int _mode = 0;
  std::shared_ptr<void> _runner;
};

}  // namespace spotify
