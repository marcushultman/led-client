#include "text_prompt.h"

#include <charconv>
#include <queue>

#include "font/font.h"
#include "spotiled.h"
#include "util/presenter.h"

namespace {

constexpr auto kDelayHeader = "delay";
constexpr auto kDefaultTimeout = std::chrono::seconds{5};

std::chrono::milliseconds timeout(const http::Request &req) {
  if (auto it = req.headers.find(kDelayHeader); it != req.headers.end()) {
    int delay_s = 0;
    auto value = std::string_view(it->second);
    std::from_chars(value.begin(), value.end(), delay_s);
    return std::chrono::seconds(delay_s);
  }
  return kDefaultTimeout;
}

}  // namespace

TextService::TextService(async::Scheduler &main_scheduler,
                         SpotiLED &led,
                         presenter::PresenterQueue &presenter,
                         BrightnessProvider &brightness_provider)
    : _main_scheduler{main_scheduler},
      _led{led},
      _presenter{presenter},
      _brightness_provider{brightness_provider} {}

http::Response TextService::handleRequest(http::Request req) {
  if (req.method != http::Method::POST) {
    return 400;
  }
  _requests.push(std::move(req));
  _presenter.add(*this, {.prio = presenter::Prio::kNotification});

  return 204;
}

void TextService::start(SpotiLED &, presenter::Callback callback) {
  auto req = std::move(_requests.front());
  _requests.pop();

  auto text = std::string(req.body.substr(req.body.find_first_of("=") + 1));
  std::transform(text.begin(), text.end(), text.begin(), ::toupper);
  _page->setText(std::move(text));

  _lifetime = _main_scheduler.schedule(
      [this,
       rolling_presenter = std::shared_ptr<RollingPresenter>(RollingPresenter::create(
           _main_scheduler, _led, _brightness_provider, *_page, Direction::kHorizontal, {})),
       callback = std::move(callback)]() mutable {
        _led.clear();
        _led.show();
        rolling_presenter.reset();
        callback();
      },
      {.delay = timeout(req)});
}

void TextService::stop() { _lifetime.reset(); }
