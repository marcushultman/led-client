#include <execinfo.h>
#include <unistd.h>

#include <cpptrace/cpptrace.hpp>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>

#include "apps/draw/draw_service.h"
#include "apps/flag/flag_service.h"
#include "apps/settings/display.h"
#include "apps/spotify/service.h"
#include "apps/text/text_service.h"
#include "apps/ui/ui_service.h"
#include "apps/web_proxy/web_proxy.h"
#include "http/http.h"
#include "http/server/server.h"
#include "present/presenter.h"
#include "util/csignal/signal_handler.h"
#include "util/spotiled/brightness_provider.h"
#include "util/spotiled/spotiled.h"
#include "util/url/url.h"

struct Options {
  bool verbose = false;
  std::string base_url;
};

Options parseOptions(int argc, char *argv[]) {
  Options opts;
  for (auto i = 0; i < argc; ++i) {
    auto arg = std::string_view(argv[i]);
    if (arg.find("--verbose") == 0) {
      opts.verbose = true;
    } else if (arg.find("--base-url") == 0) {
      opts.base_url = arg.substr(11);
    }
  }
  return opts;
}

struct PathMapper {
  using Map = std::unordered_map<std::string, http::SyncHandler>;

  PathMapper(Map map, http::AsyncHandler index = {})
      : _map{std::move(map)}, _index{std::move(index)} {}

  http::Lifetime operator()(http::Request req,
                            http::RequestOptions::OnResponse on_response,
                            http::RequestOptions::OnBytes on_bytes) {
    url::Url url(req.url);
    auto it = url.path.empty() ? _map.end() : _map.find(std::string(url.path.front()));
    if (it != _map.end()) {
      on_response(it->second(std::move(req)));
      return {};
    }
    return _index(req, std::move(on_response), std::move(on_bytes));
  }

 private:
  Map _map;
  http::AsyncHandler _index;
};

int main(int argc, char *argv[]) {
  auto http = http::Http::create();
  if (!http) {
    return 1;
  }

  auto opts = parseOptions(argc, argv);
  auto main_thread = async::Thread::create("main");
  auto &main_scheduler = main_thread->scheduler();
  auto brightness = spotiled::BrightnessProvider();
  auto presenter = present::makePresenterQueue(main_scheduler, brightness);

  auto display_service = settings::DisplayService(main_scheduler, brightness, *presenter);
  auto flag_service = std::make_unique<FlagService>(*presenter);
  auto text_service = std::make_unique<TextService>(main_scheduler, *presenter);
  auto ui_service = makeUIService(main_scheduler, *http, *presenter, opts.base_url);
  auto spotify_service =
      std::make_unique<spotify::SpotifyService>(main_scheduler, *http, *presenter, opts.verbose);
  auto web_proxy =
      web_proxy::WebProxy(main_scheduler, *http, brightness, *spotify_service, opts.base_url);

  auto drawer = std::make_unique<DrawService>(main_scheduler, *presenter);

  // todo: proxy and route settings

  PathMapper mapper{
      {{"ping", [](auto) { return 200; }},
       {"text", [&](auto req) { return text_service->handleRequest(std::move(req)); }},
       {"ui", [&](auto req) { return ui_service->handleRequest(std::move(req)); }},
       {"spotify", [&](auto req) { return spotify_service->handleRequest(std::move(req)); }},
       {"murica", [&](auto req) { return flag_service->handleRequest(std::move(req)); }},
       {"settings", [&](auto req) { return display_service(std::move(req)); }},
       {"draw", [&](auto req) { return drawer->handleRequest(std::move(req)); }}},
      [&](auto req, auto on_response, auto on_bytes) {
        return web_proxy.handleRequest(std::move(req), std::move(on_response), std::move(on_bytes));
      },
  };

  auto server = http::makeServer(main_scheduler, mapper);
  std::cout << "Listening on port: " << server->port() << std::endl;

  auto interrupt = std::promise<int>();
  auto sig = csignal::SignalHandler(main_scheduler, [&](auto signal) {
    std::cerr << "Signal received: " << signal << std::endl;

    if (signal == SIGINT) {
      presenter->clear();
      server.reset();
      interrupt.set_value(0);
    }
    return signal == SIGINT;
  });

  std::signal(SIGSEGV, [](auto) {
    cpptrace::generate_trace().print();
    std::signal(SIGSEGV, nullptr);
    std::raise(SIGSEGV);
  });

  const auto status = interrupt.get_future().get();
  std::cout << "Exiting with status code: " << status << std::endl;
  return status;
}
