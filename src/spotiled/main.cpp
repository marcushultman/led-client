#include <csignal/signal_catcher.h>
#include <execinfo.h>
#include <program_options/program_options.h>
#include <unistd.h>

#include <csignal>
#include <future>
#include <iostream>

#include "http/http.h"
#include "http/server/server.h"
#include "spotiled/spotiled.h"
#include "web_proxy/web_proxy.h"

struct Stack {
  std::unique_ptr<web_proxy::WebProxy> web_proxy;
  std::unique_ptr<http::Server> server;
  std::unique_ptr<csignal::SignalCatcher> signal;
};

int main(int argc, char *argv[]) {
  auto http = http::Http::create();
  if (!http) {
    return 1;
  }

  auto opts = program_options::parseOptions(argc, argv);
  auto main_thread = async::Thread::create("main");
  auto &main_scheduler = main_thread->scheduler();
  auto settings = settings::Settings{.brightness = 0x3F, .hue = 0xFF};

  auto stack = std::make_unique<Stack>();
  auto interrupt = std::promise<int>();

  auto _ = main_scheduler.schedule([&] {
    stack->web_proxy = std::make_unique<web_proxy::WebProxy>(
        main_scheduler, *http, spotiled::create(main_scheduler, settings), opts.base_url,
        "spotiled");

    stack->server = http::makeServer(main_scheduler, stack->web_proxy->asRequestHandler());
    std::cout << "Listening on port: " << stack->server->port() << std::endl;

    stack->signal = std::make_unique<csignal::SignalCatcher>(
        main_scheduler, std::vector{SIGINT}, [&](auto signal) {
          std::cerr << "Signal received: " << signal << std::endl;
          stack.reset();
          interrupt.set_value(0);
        });
  });

  const auto status = interrupt.get_future().get();
  std::cout << "Exiting with status code: " << status << std::endl;
  return status;
}
