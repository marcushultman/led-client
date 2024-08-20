#include <execinfo.h>
#include <program_options/program_options.h>
#include <unistd.h>

#include <csignal>
#include <future>
#include <iostream>

#include "csignal/signal_handler.h"
#include "http/http.h"
#include "http/server/server.h"
#include "ikea/ikea.h"
#include "present/presenter.h"
#include "settings/updater.h"
#include "web_proxy/web_proxy.h"

int main(int argc, char *argv[]) {
  auto http = http::Http::create();
  if (!http) {
    return 1;
  }

  auto opts = program_options::parseOptions(argc, argv);
  auto main_thread = async::Thread::create("main");
  auto &main_scheduler = main_thread->scheduler();
  auto settings = settings::Settings();

  auto presenter = present::makePresenter(opts.ikea ? ikea::create(main_scheduler, settings)
                                                    : spotiled::create(main_scheduler, settings));

  auto web_proxy = std::make_unique<web_proxy::WebProxy>(
      main_scheduler, *http, *presenter, opts.base_url, opts.ikea ? "ikea" : "spotiled",
      web_proxy::StateThingy::Callbacks{
          {"/settings2", [&](auto data) { settings::updateSettings(settings, data); }}});

  auto server = http::makeServer(main_scheduler, web_proxy->asRequestHandler());
  auto _ = main_scheduler.schedule(
      [port = server->port()] { std::cout << "Listening on port: " << port << std::endl; });

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
