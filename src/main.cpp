#include <stdio.h>

#include <csignal>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>

#include "http/http.h"
#include "server/server.h"
#include "spotify_client.h"
#include "spotiled.h"

struct SignalHandler {
  using Callback = std::function<void(int)>;

  SignalHandler(async::Scheduler &scheduler, Callback callback)
      : _scheduler{scheduler}, _callback{std::move(callback)} {
    if (s_handler) {
      std::terminate();
    }
    s_handler = this;
    std::signal(SIGINT, [](int sig) { s_handler->schedule(sig); });
  }
  ~SignalHandler() {
    std::signal(SIGINT, nullptr);
    s_handler = nullptr;
  }

  void schedule(int sig) {
    _lifetime = _scheduler.schedule([this, sig] { _callback(sig); });
  }

  static SignalHandler *s_handler;
  async::Scheduler &_scheduler;
  Callback _callback;
  async::Lifetime _lifetime;
};

SignalHandler *SignalHandler::s_handler = nullptr;

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

  auto main_thread = async::Thread::create("main");
  auto led = SpotiLED::create();

  auto client = SpotifyClient::create(main_thread->scheduler(), *http, *led, brightness, verbose);

  if (!client) {
    return 1;
  }

  auto server = makeServer();
  std::cout << "Listening on port: " << server->port() << std::endl;

  auto interrupt = std::promise<int>();
  auto sig = SignalHandler(main_thread->scheduler(), [&](auto) {
    server.reset();
    interrupt.set_value(0);
  });

  return interrupt.get_future().get();
}
