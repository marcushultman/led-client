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

  auto brightness_provider = BrightnessProvider::create(brightness);
  auto main_thread = async::Thread::create("main");
  auto led = SpotiLED::create();
  auto &main_scheduler = main_thread->scheduler();

  // for ad-hoc text rendering
  auto page = font::TextPage::create();

  // mode
  int mode = 0;
  std::shared_ptr<void> runner;
  auto next_mode = [&](ServerRequest req) {
    if (req.method == "POST" && req.body.find("text") == 0) {
      auto text = std::string(req.body.substr(req.body.find_first_of("=") + 1));
      std::transform(text.begin(), text.end(), text.begin(), ::toupper);
      page->setText(text);

      std::vector<async::Lifetime> lifetimes;

      if (auto it = req.headers.find("delay"); it != req.headers.end()) {
        int delay_s = 0;
        std::from_chars(it->second.begin(), it->second.end(), delay_s);
        lifetimes.push_back(main_scheduler.schedule(
            [&] {
              runner.reset();
              led->clear();
              led->show();
            },
            {.delay = std::chrono::seconds(delay_s)}));
      }

      lifetimes.push_back(RollingPresenter::create(main_scheduler, *led, *brightness_provider,
                                                   *page, Direction::kHorizontal, {}));

      runner = std::make_shared<std::vector<async::Lifetime>>(std::move(lifetimes));
      return;
    }
    if (req.path != "/mode") {
      return;
    }

    if (auto it = req.headers.find("action"); it != req.headers.end()) {
      std::from_chars(it->second.begin(), it->second.end(), mode);
    } else {
      mode = (mode + 1) % 2;
    }

    switch (mode) {
      case 0:
        led->clear();
        led->show();
        return runner.reset();
      case 1:
        runner = SpotifyClient::create(main_scheduler, *http, *led, *brightness_provider, verbose);
        return;
    }
  };
  next_mode(ServerRequest{});

  auto server = makeServer(main_scheduler, next_mode);
  std::cout << "Listening on port: " << server->port() << std::endl;

  auto interrupt = std::promise<int>();
  auto sig = SignalHandler(main_scheduler, [&](auto) {
    server.reset();
    interrupt.set_value(0);
  });

  return interrupt.get_future().get();
}
