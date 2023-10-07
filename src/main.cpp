#include <cstdio>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>

#include "apps/settings/brightness_provider.h"
#include "apps/settings/display.h"
#include "apps/spotify/service.h"
#include "apps/text/text_service.h"
#include "http/http.h"
#include "http/server/server.h"
#include "present/presenter.h"
#include "util/csignal/signal_handler.h"
#include "util/spotiled/spotiled.h"

struct Options {
  bool verbose = false;
};

Options parseOptions(int argc, char *argv[]) {
  Options opts;
  for (auto i = 0; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.find("--verbose") == 0) {
      opts.verbose = true;
    }
  }
  return opts;
}

struct PathMapper {
  using Map = std::unordered_map<std::string, std::function<http::Response(http::Request)>>;

  PathMapper(Map map) : _map{std::move(map)} {}

  http::Response operator()(http::Request req) {
    url::Url url(req.url);
    auto it = url.path.empty() ? _map.end() : _map.find(std::string(url.path.front()));
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
  auto main_thread = async::Thread::create("main");
  auto &main_scheduler = main_thread->scheduler();
  auto led = SpotiLED::create(main_scheduler);
  auto presenter = present::makePresenterQueue(*led);

  auto display_service = settings::DisplayService(main_scheduler, *presenter);
  auto text_service =
      std::make_unique<TextService>(main_scheduler, *led, *presenter, display_service);
  auto spotify_service = std::make_unique<spotify::SpotifyService>(
      main_scheduler, *http, *led, *presenter, display_service, opts.verbose);

  // todo: proxy and route settings

  PathMapper mapper{{
      {"ping", [](auto) { return 200; }},
      {"text", [&](auto req) { return text_service->handleRequest(std::move(req)); }},
      {"mode", [&](auto req) { return spotify_service->handleRequest(std::move(req)); }},
      {"settings", [&](auto req) { return display_service(std::move(req)); }},
  }};

  std::cout << "Using logo brightness: " << int(display_service.logoBrightness()[0])
            << ", brightness: " << int(display_service.brightness()[0]) << std::endl;

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
