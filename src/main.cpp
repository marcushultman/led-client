#include <cstdio>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>

#include "apps/settings/display.h"
#include "apps/spotify/service.h"
#include "http/http.h"
#include "http/server/server.h"
#include "spotiled.h"
#include "util/presenter.h"
#include "util/signal_handler.h"
#include "util/text_prompt.h"

struct Options {
  bool verbose = false;
  uint8_t brightness = 1;
};

Options parseOptions(int argc, char *argv[]) {
  Options opts;
  for (auto i = 0; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.find("--verbose") == 0) {
      opts.verbose = true;
    } else if (arg.find("--brightness") == 0) {
      // max 32 to avoid power brownout
      opts.brightness = std::clamp(std::atoi(arg.data() + 13), 1, 32);
    }
  }
  return opts;
}

struct PathMapper {
  using Map = std::unordered_map<std::string, std::function<http::Response(http::Request)>>;

  PathMapper(Map map) : _map{std::move(map)} {}

  http::Response operator()(http::Request req) {
    url::Url url(req.url);
    auto it = url.path.empty() ? _map.end() : _map.find(std::string(url.path.back()));
    if (it != _map.end()) {
      return it->second(std::move(req));
    }
    return 404;
  }

 private:
  Map _map;
};

int main(int argc, char *argv[]) {
  auto http = http::Http::create();
  if (!http) {
    return 1;
  }

  auto opts = parseOptions(argc, argv);
  auto brightness_provider = BrightnessProvider::create(opts.brightness);
  auto main_thread = async::Thread::create("main");
  auto led = SpotiLED::create();
  auto &main_scheduler = main_thread->scheduler();
  auto presenter = presenter::makePresenterQueue(*led);

  std::cout << "Using logo brightness: " << int(brightness_provider->logoBrightness()[0])
            << ", brightness: " << int(brightness_provider->brightness()[0]) << std::endl;

  auto text_service =
      std::make_unique<TextService>(main_scheduler, *led, *presenter, *brightness_provider);
  auto spotify_service = std::make_unique<spotify::SpotifyService>(
      main_scheduler, *http, *led, *presenter, *brightness_provider, opts.verbose);
  auto display_service = settings::DisplayService();

  // todo: proxy and route settings

  PathMapper mapper{{
      {"text", [&](auto req) { return text_service->handleRequest(std::move(req)); }},
      {"mode", [&](auto req) { return spotify_service->handleRequest(std::move(req)); }},
      {"settings", display_service},
  }};

  auto server = http::makeServer(main_scheduler, mapper);
  std::cout << "Listening on port: " << server->port() << std::endl;

  auto interrupt = std::promise<int>();
  auto sig = csignal::SignalHandler(main_scheduler, [&](auto) {
    presenter->clear();
    server.reset();
    interrupt.set_value(0);
  });

  return interrupt.get_future().get();
}
