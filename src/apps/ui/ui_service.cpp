#include "apps/ui/ui_service.h"

#include <charconv>
#include <iostream>

#include "util/url/url.h"

namespace {

using namespace std::chrono_literals;
using namespace std::string_view_literals;

const auto kDefaultBaseUrl = "https://spotiled.deno.dev"sv;

void hexToColor(std::string_view hex, Color &color) {
  std::from_chars(&hex[0], &hex[2], color.r(), 16);
  std::from_chars(&hex[2], &hex[4], color.g(), 16);
  std::from_chars(&hex[4], &hex[6], color.b(), 16);
}

auto findHexColor(http::Headers &headers, const std::string &key) {
  auto it = headers.find("x-spotiled-logo");
  return it != headers.end() && it->second.size() == 6 ? &it->second : nullptr;
}

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
  Color _logo = color::kWhite;
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
  if (_lifetime) {
    _lifetime.reset();
    _bytes.reset();
    _presenter.erase(*this);
    _renderer.notify();
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
                       if (std::exchange(_bytes, std::move(res.body))) {
                         _renderer.notify();
                       } else {
                         _presenter.add(*this);
                       }
                       auto *color = findHexColor(res.headers, "x-spotiled-logo");
                       color ? hexToColor(*color, _logo) : void(_logo = color::kWhite);
                     }});
  return 204;
}

void UIServiceImpl::onStart() {
  _renderer.add([this](spotiled::LED &led, auto elapsed) -> std::chrono::milliseconds {
    if (!_bytes) {
      return {};
    }

    led.setLogo(_logo);

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
