
#include <chrono>

// #include "util/spotiled/spotiled.h"

int main() {
  // for (size_t i = 0; i < 20; ++i) {
  //   auto ms = requestBackoff(i);
  //   auto s = std::chrono::duration_cast<std::chrono::seconds>(ms);
  //   printf("%ds\n", int(s.count()));
  // }

  // auto http = http::Http::create();
  // auto thread = async::Thread::create("main");
  // auto led = spotiled::LED::create();
  // auto presenter_queue = present::makePresenterQueue(*led);

  // auto client =
  //     SpotifyClient::create(thread->scheduler(), *http, *led, *presenter_queue, brightness,
  //     false);

  // async::Lifetime lifetime;
  // lifetime = thread->scheduler().schedule([&] {
  //   client.reset();
  //   http.reset();
  //   lifetime.reset();
  // });
}
