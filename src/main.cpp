#include <stdio.h>

#include <charconv>
#include <csignal>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include "font/font.h"
#include "http/http.h"
#include "http/server/server.h"
#include "service/service_registry.h"
#include "service/service_runner.h"
#include "spotify_client.h"
#include "spotiled.h"
#include "util/signal_handler.h"
#include "util/text_prompt.h"
#include "util/url.h"

auto kDefaultHttpResponse = http::Response(
    "<html><body><form action=\"/text\" method=\"post\"><input name=\"text\" "
    "placeholder=\"\"><input type=\"submit\"></form></body></html>");

struct SpotifyService : service::API {
  SpotifyService(async::Scheduler &main_scheduler,
                 http::Http &http,
                 SpotiLED &led,
                 BrightnessProvider &brightness_provider,
                 bool verbose)
      : _main_scheduler(main_scheduler),
        _http(http),
        _led(led),
        _brightness_provider(brightness_provider),
        _verbose(verbose) {}

  http::Response handleRequest(http::Request req) final {
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
        _led.clear();
        _led.show();
        _runner.reset();
        return kDefaultHttpResponse;
      case 1:
        _runner =
            SpotifyClient::create(_main_scheduler, _http, _led, _brightness_provider, _verbose);
        return kDefaultHttpResponse;
    }
    return 204;
  }

 private:
  async::Scheduler &_main_scheduler;
  http::Http &_http;
  SpotiLED &_led;
  BrightnessProvider &_brightness_provider;
  bool _verbose;

  int _mode = 0;
  std::shared_ptr<void> _runner;
};

int main(int argc, char *argv[]) {
  auto http = http::Http::create();
  if (!http) {
    return 1;
  }
  auto verbose = false;
  uint8_t brightness = 1;

  for (auto i = 0; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.find("--verbose") == 0) {
      verbose = true;
    } else if (arg.find("--brightness") == 0) {
      // max 32 to avoid power brownout
      brightness = std::clamp(std::atoi(arg.data() + 13), 1, 32);
    }
  }

  auto brightness_provider = BrightnessProvider::create(brightness);
  auto main_thread = async::Thread::create("main");
  auto led = SpotiLED::create();
  auto &main_scheduler = main_thread->scheduler();

  auto services = service::makeServices({
      {"text",
       [&](auto &) {
         return std::make_unique<TextPrompt>(main_scheduler, *led, *brightness_provider);
       }},
      {"mode",
       [&](auto &) {
         return std::make_unique<SpotifyService>(main_scheduler, *http, *led, *brightness_provider,
                                                 verbose);
       }},
  });
  auto service_runner = service::makeServiceRunner(main_scheduler, *services);

  auto server = http::makeServer(main_scheduler, [&](auto req) mutable {
    return service_runner->handleRequest(std::move(req)).value_or(404);
  });
  std::cout << "Listening on port: " << server->port() << std::endl;

  auto interrupt = std::promise<int>();
  auto sig = csignal::SignalHandler(main_scheduler, [&](auto) {
    server.reset();
    interrupt.set_value(0);
  });

  return interrupt.get_future().get();
}
