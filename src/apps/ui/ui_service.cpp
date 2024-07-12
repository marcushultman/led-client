#include "apps/ui/ui_service.h"

#include <charconv>
#include <iostream>

#include "util/url/url.h"

namespace {

using namespace std::chrono_literals;
using namespace std::string_view_literals;

const auto kDefaultBaseUrl = "https://spotiled.deno.dev"sv;

auto hexToColor(std::string_view hex) {
  Color color;
  std::from_chars(&hex[0], &hex[2], color.r(), 16);
  std::from_chars(&hex[2], &hex[4], color.g(), 16);
  std::from_chars(&hex[4], &hex[6], color.b(), 16);
  return color;
}

auto findHexColor(http::Headers &headers, const std::string &key) {
  auto it = headers.find("x-spotiled-logo");
  return it != headers.end() && it->second.size() == 6 ? &it->second : nullptr;
}

}  // namespace

struct UIServiceImpl final : UIService {
  UIServiceImpl(async::Scheduler &,
                http::Http &,
                present::PresenterQueue &,
                std::string_view base_url);

  virtual http::Response handleRequest(http::Request) final;

 private:
  struct RenderState {
    Color logo = color::kWhite;
    std::string bytes;
    std::chrono::milliseconds expire_in = {};
  };

  void onResponse(std::string_view path, http::Response res);
  void decorateState(RenderState &, http::Response res);

  void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed) final;

  void stop();

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  present::PresenterQueue &_presenter;
  std::string_view _base_url;

  http::Lifetime _lifetime;
  std::optional<RenderState> _render_state;
};

UIServiceImpl::UIServiceImpl(async::Scheduler &main_scheduler,
                             http::Http &http,
                             present::PresenterQueue &presenter,
                             std::string_view base_url)
    : _main_scheduler{main_scheduler},
      _http{http},
      _presenter{presenter},
      _base_url{base_url.empty() ? kDefaultBaseUrl : base_url} {}

http::Response UIServiceImpl::handleRequest(http::Request req) {
  if (_lifetime) {
    stop();
    return 204;
  }
  auto url = url::Url(req.url);
  auto path = std::string(url.path[0].end(), url.end());
  _lifetime = _http.request({.url = std::string(_base_url) + path},
                            {.post_to = _main_scheduler, .on_response = [this, path](auto res) {
                               onResponse(path, std::move(res));
                             }});
  return 204;
}

void UIServiceImpl::onResponse(std::string_view path, http::Response res) {
  if (res.status / 100 != 2) {
    std::cout << "ui_service: " << path << " status: " << res.status << std::endl;
    stop();
  } else if (_render_state) {
    decorateState(*_render_state, std::move(res));
    _presenter.notify();
  } else {
    _render_state = RenderState();
    decorateState(*_render_state, std::move(res));
    _presenter.add(*this);
  }
}

void UIServiceImpl::decorateState(RenderState &state, http::Response res) {
  auto *color = findHexColor(res.headers, "x-spotiled-logo");
  state.logo = color ? hexToColor(*color) : color::kWhite;

  state.bytes = std::move(res.body);

  if (auto it = res.headers.find("x-spotiled-ttl"); it != res.headers.end()) {
    auto ttl_seconds = int(0);
    std::from_chars(it->second.data(), it->second.data() + it->second.size(), ttl_seconds);
    state.expire_in = std::chrono::seconds{ttl_seconds};
  }
}

void UIServiceImpl::onRenderPass(spotiled::LED &led, std::chrono::milliseconds elapsed) {
  if (elapsed > _render_state->expire_in) {
    stop();
    return;
  }

  led.setLogo(_render_state->logo);

  auto bytes = std::string_view(_render_state->bytes);
  for (auto i = 0; i < bytes.size() / 4; ++i) {
    auto *p = &bytes[i * 4];
    auto [r, g, b, a] = std::tie(p[0], p[1], p[2], p[3]);
    led.set({i / 16, i % 16}, Color(r, g, b), {.src = uint8_t(a) / 255.0});
  }
}

void UIServiceImpl::stop() {
  _lifetime.reset();
  if (_render_state) {
    _presenter.erase(*this);
    _render_state = {};
  }
}

std::unique_ptr<UIService> makeUIService(async::Scheduler &main_scheduler,
                                         http::Http &http,
                                         present::PresenterQueue &presenter,
                                         std::string_view base_url) {
  return std::make_unique<UIServiceImpl>(main_scheduler, http, presenter, base_url);
}
