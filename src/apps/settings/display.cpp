#include "display.h"

#include "spotiled.h"

namespace settings {

DisplayService::DisplayService(present::PresenterQueue &presenter) : _presenter{presenter} {}

http::Response DisplayService::operator()(http::Request req) {
  if (req.method != http::Method::POST || req.body.empty()) {
    return 400;
  }
  _brightness = std::stoi(req.body);
  _presenter.add(*this);
  return 204;
}

void DisplayService::start(SpotiLED &led, present::Callback callback) {
  led.clear();
  led.setLogo(_brightness);
  for (auto i = 0; i < 23; ++i) {
    led.set({i, 8}, _brightness);
    led.set({i, 9}, _brightness);
  }
  led.show();
  callback();
}

void DisplayService::stop() {}

}  // namespace settings
