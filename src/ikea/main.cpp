#include <csignal/signal_catcher.h>
#include <execinfo.h>
#include <ikea/button_reader.h>
#include <program_options/program_options.h>
#include <unistd.h>

#include <csignal>
#include <future>
#include <iostream>

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

  auto presenter = present::makePresenter(ikea::create(main_scheduler, settings));

  auto web_proxy = std::make_unique<web_proxy::WebProxy>(
      main_scheduler, *http, *presenter, opts.base_url, "ikea",
      web_proxy::StateThingy::Callbacks{
          {"/settings2", [&](auto data) { settings::updateSettings(settings, data); }}});

  auto button_reader = std::make_unique<ikea::ButtonReader>(
      main_scheduler, [&] { web_proxy->updateState("/button"); });

  auto server = http::makeServer(main_scheduler, web_proxy->asRequestHandler());
  auto _ = main_scheduler.schedule(
      [port = server->port()] { std::cout << "Listening on port: " << port << std::endl; });

  auto interrupt = std::promise<int>();
  auto sig = csignal::SignalCatcher(main_scheduler, {SIGINT}, [&](auto signal) {
    std::cerr << "Signal received: " << signal << std::endl;
    server.reset();
    web_proxy.reset();
    presenter.reset();
    interrupt.set_value(0);
  });

  const auto status = interrupt.get_future().get();
  std::cout << "Exiting with status code: " << status << std::endl;
  return status;
}
