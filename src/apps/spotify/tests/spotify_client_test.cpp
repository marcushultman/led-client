#include "apps/spotify/spotify_client.h"

#include <chrono>

#include "util/spotiled/spotiled.h"

struct BrightnessProvider : settings::BrightnessProvider {
  Color logoBrightness() const final { return 0; }
  Color brightness() const final { return 0; }
};

int main() {
  for (size_t i = 0; i < 20; ++i) {
    auto ms = spotify::requestBackoff(i);
    auto s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    printf("%ds\n", int(s.count()));
  }

  // auto http = http::Http::create();
  // auto thread = async::Thread::create("main");
  // auto led = spotiled::LED::create();
  // auto presenter_queue = present::makePresenterQueue(*led);
  // BrightnessProvider brightness;

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
