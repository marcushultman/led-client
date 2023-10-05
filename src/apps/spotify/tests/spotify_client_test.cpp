#include "apps/spotify/spotify_client.h"

#include "util/spotiled/spotiled.h"

struct BrightnessProvider : settings::BrightnessProvider {
  Color logoBrightness() const final { return 0; }
  Color brightness() const final { return 0; }
};

int main() {
  auto http = http::Http::create();
  auto thread = async::Thread::create("main");
  auto led = SpotiLED::create();
  auto presenter_queue = present::makePresenterQueue(*led);
  BrightnessProvider brightness;

  auto client =
      SpotifyClient::create(thread->scheduler(), *http, *led, *presenter_queue, brightness, false);

  async::Lifetime lifetime;
  lifetime = thread->scheduler().schedule([&] {
    client.reset();
    http.reset();
    lifetime.reset();
  });
}
