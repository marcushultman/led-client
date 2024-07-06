#include "apps/ui/ui_service.h"

#include <iostream>

#include "util/url/url.h"

namespace {

using namespace std::chrono_literals;
using namespace std::string_view_literals;

const auto kDefaultBaseUrl = "https://spotiled.deno.dev"sv;

}  // namespace

struct UIServiceImpl final : UIService {
  UIServiceImpl(async::Scheduler &,
                http::Http &,
                spotiled::Renderer &,
                present::PresenterQueue &,
                std::string_view base_url);

  virtual http::Response handleRequest(http::Request) final;

  void onStart() final;
  void onStop() final;

 private:
  void onResponse(const std::string &json);

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  spotiled::Renderer &_renderer;
  present::PresenterQueue &_presenter;
  std::string_view _base_url;

  std::optional<std::string> _bytes;
  http::Lifetime _lifetime;
};

UIServiceImpl::UIServiceImpl(async::Scheduler &main_scheduler,
                             http::Http &http,
                             spotiled::Renderer &renderer,
                             present::PresenterQueue &presenter,
                             std::string_view base_url)
    : _main_scheduler{main_scheduler},
      _http{http},
      _renderer{renderer},
      _presenter{presenter},
      _base_url{base_url.empty() ? kDefaultBaseUrl : base_url} {}

http::Response UIServiceImpl::handleRequest(http::Request req) {
  if (!req.body.starts_with("mode=")) {
    return 400;
  }
  if (!req.body.ends_with("=true")) {
    _lifetime.reset();
    _bytes.reset();
    _presenter.erase(*this);
    return 204;
  }
  auto url = url::Url(req.url);
  _lifetime =
      _http.request({.url = std::string(_base_url) + std::string(url.path[0].end(), url.end())},
                    {.post_to = _main_scheduler, .on_response = [this](auto res) {
                       if (res.status / 100 != 2) {
                         _bytes.reset();
                         return;
                       }
                       if (!_bytes) {
                         _presenter.add(*this);
                       }
                       _bytes = std::move(res.body);
                       _renderer.notify();
                     }});
  return 204;
}

void UIServiceImpl::onStart() {
  _renderer.add([this](spotiled::LED &led, auto elapsed) -> std::chrono::milliseconds {
    if (!_bytes) {
      return {};
    }

    auto bytes = std::string_view(*_bytes);
    for (auto i = 0; i < bytes.size() / 4; ++i) {
      auto *p = &bytes[i * 4];
      auto [r, g, b, a] = std::tie(p[0], p[1], p[2], p[3]);
      led.set({i / 16, i % 16}, Color(r, g, b), {.src = uint8_t(a) / 255.0});
    }

    return 1h;
  });
}

void UIServiceImpl::onStop() { _bytes.reset(); }

std::unique_ptr<UIService> makeUIService(async::Scheduler &main_scheduler,
                                         http::Http &http,
                                         spotiled::Renderer &renderer,
                                         present::PresenterQueue &presenter,
                                         std::string_view base_url) {
  return std::make_unique<UIServiceImpl>(main_scheduler, http, renderer, presenter, base_url);
}
