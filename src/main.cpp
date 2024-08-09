#include <execinfo.h>
#include <unistd.h>

#include <csignal>
#include <future>
#include <iostream>
#include <string>

#include "apps/settings/display.h"
#include "apps/web_proxy/web_proxy.h"
#include "csignal/signal_handler.h"
#include "http/http.h"
#include "http/server/server.h"
#include "present/presenter.h"
#include "spotiled/brightness_provider.h"
#include "uri/uri.h"

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
  using Map = std::unordered_map<std::string, http::RequestHandler>;

  PathMapper(Map map, http::AsyncHandler index = {})
      : _map{std::move(map)}, _index{std::move(index)} {}

  http::Lifetime operator()(http::Request req,
                            http::RequestOptions::OnResponse on_response,
                            http::RequestOptions::OnBytes on_bytes) {
    auto key = std::string(uri::Uri::Path(req.url).front());
    if (auto it = _map.find(key); it != _map.end()) {
      if (auto *handler = std::get_if<http::AsyncHandler>(&it->second)) {
        return (*handler)(std::move(req), std::move(on_response), std::move(on_bytes));
      }
      on_response(std::get<http::SyncHandler>(it->second)(std::move(req)));
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
  auto web_proxy = std::make_unique<web_proxy::WebProxy>(main_scheduler, *http, brightness,
                                                         *presenter, opts.base_url);

  // todo: proxy and route settings

  PathMapper mapper{
      {{"settings", [&](auto req) { return display_service(std::move(req)); }}},
      [&](auto req, auto on_response, auto on_bytes) {
        return web_proxy->handleRequest(std::move(req), std::move(on_response),
                                        std::move(on_bytes));
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
      web_proxy.reset();
      interrupt.set_value(0);
    }
    return signal == SIGINT;
  });

  const auto status = interrupt.get_future().get();
  std::cout << "Exiting with status code: " << status << std::endl;
  return status;
}
