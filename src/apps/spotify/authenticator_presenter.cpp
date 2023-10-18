#include "authenticator_presenter.h"

#include <chrono>
#include <iostream>

#include "apps/settings/brightness_provider.h"
#include "credentials.h"
#include "font/font.h"
#include "jq_util.h"
#include "now_playing_service.h"
#include "present/rolling_presenter.h"
#include "util/spotiled/spotiled.h"

extern "C" {
#include <jq.h>
}

namespace spotify {
namespace {

const auto kAuthDeviceCodeUrl = "https://accounts.spotify.com/api/device/code";
const auto kAuthTokenUrl = "https://accounts.spotify.com/api/token";

const auto kContentType = "content-type";
const auto kXWWWFormUrlencoded = "application/x-www-form-urlencoded";

void nextSeconds(jq_state *jq, std::chrono::seconds &value) {
  using sec = std::chrono::seconds;
  value = sec{static_cast<sec::rep>(nextNumber(jq))};
}

struct DeviceFlowData {
  std::string device_code;
  std::string user_code;
  std::chrono::seconds expires_in;
  std::string verification_url;
  std::string verification_url_prefilled;
  std::chrono::seconds interval;
};

void parseDeviceFlowData(jq_state *jq, const std::string &buffer, DeviceFlowData &data) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq,
             ".device_code, .user_code, .expires_in, .verification_url, "
             ".verification_url_prefilled, .interval");
  jq_start(jq, input, 0);
  nextStr(jq, data.device_code);
  nextStr(jq, data.user_code);
  nextSeconds(jq, data.expires_in);
  nextStr(jq, data.verification_url);
  nextStr(jq, data.verification_url_prefilled);
  nextSeconds(jq, data.interval);
}

struct TokenData {
  std::string error;
  std::string access_token;
  // std::string token_type;  // "Bearer";
  // std::chrono::seconds expires_in;  // 3600;
  std::string refresh_token;
  // std::string scope;
  // std::string client_id;
  // std::string client_secret;
};

void parseTokenData(jq_state *jq, const std::string &buffer, TokenData &data) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq, ".error, .access_token, .refresh_token");
  jq_start(jq, input, 0);
  nextStr(jq, data.error);
  nextStr(jq, data.access_token);
  nextStr(jq, data.refresh_token);
}

}  // namespace

class AuthenticatorPresenterImpl final : public AuthenticatorPresenter, present::Presentable {
 public:
  AuthenticatorPresenterImpl(async::Scheduler &main_scheduler,
                             http::Http &http,
                             present::PresenterQueue &presenter,
                             settings::BrightnessProvider &brightness,
                             jq_state *jq,
                             bool verbose,
                             AccessTokenCallback token_callback)
      : _main_scheduler{main_scheduler},
        _http{http},
        _presenter{presenter},
        _brightness{brightness},
        _jq{jq},
        _token_callback{std::move(token_callback)} {
    _presenter.add(*this);
    authenticate();
  }
  ~AuthenticatorPresenterImpl() { jq_teardown(&_jq); }

  void start(spotiled::Renderer &, present::Callback) final;
  void stop() final;

 private:
  void authenticate();
  void scheduleAuthRetry(std::chrono::milliseconds delay = {});

  void onAuthResponse(http::Response response);
  void pollToken();
  void onPollTokenResponse(http::Response response);
  void scheduleNextPollToken();

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  present::PresenterQueue &_presenter;
  settings::BrightnessProvider &_brightness;
  jq_state *_jq;
  AccessTokenCallback _token_callback;
  present::Callback _presenter_callback;

  struct AuthState {
    std::chrono::system_clock::time_point expiry;
    std::chrono::milliseconds interval;
    std::string device_code;
    std::string user_code;
  };
  AuthState _auth_state;

  std::unique_ptr<font::TextPage> _text = font::TextPage::create();

  http::Lifetime _request;
  async::Lifetime _work;
};

std::unique_ptr<AuthenticatorPresenter> AuthenticatorPresenter::create(
    async::Scheduler &main_scheduler,
    http::Http &http,
    present::PresenterQueue &presenter,
    settings::BrightnessProvider &brightness,
    bool verbose,
    AccessTokenCallback callback) {
  auto jq = jq_init();
  if (!jq) {
    return nullptr;
  }
  return std::make_unique<AuthenticatorPresenterImpl>(main_scheduler, http, presenter, brightness,
                                                      jq, verbose, callback);
}

void AuthenticatorPresenterImpl::start(spotiled::Renderer &renderer, present::Callback callback) {
  _presenter_callback = callback;

  renderer.add([this](auto &led, auto elapsed) {
    using namespace std::chrono_literals;
    if (!_presenter_callback) {
      return 0ms;
    }

    if (!_auth_state.user_code.empty()) {
      renderRolling(led, _brightness, elapsed, *_text);
    }

    return 200ms;
  });
}
void AuthenticatorPresenterImpl::stop() { _request.reset(); }

void AuthenticatorPresenterImpl::authenticate() {
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&scope=user-read-playback-state"
                    "&description=Spotify-LED";

  _request = _http.request({.method = http::Method::POST,
                            .url = kAuthDeviceCodeUrl,
                            .headers = {{kContentType, kXWWWFormUrlencoded}},
                            .body = data},
                           {.post_to = _main_scheduler, .callback = [this](auto response) {
                              onAuthResponse(std::move(response));
                            }});
}

void AuthenticatorPresenterImpl::scheduleAuthRetry(std::chrono::milliseconds delay) {
  _work = _main_scheduler.schedule([this] { authenticate(); }, {.delay = delay});
}

void AuthenticatorPresenterImpl::onAuthResponse(http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "failed to get device_code" << std::endl;
    scheduleAuthRetry(std::chrono::seconds{5});
    return;
  }

  DeviceFlowData data;
  parseDeviceFlowData(_jq, response.body, data);
  std::cerr << "url: " << data.verification_url_prefilled << std::endl;

  auto now = std::chrono::system_clock::now();
  _auth_state.expiry = now + data.expires_in;
  _auth_state.interval = data.interval;
  _auth_state.device_code = data.device_code;
  _auth_state.user_code = data.user_code;

  _text->setText(_auth_state.user_code);

  pollToken();
}

void AuthenticatorPresenterImpl::pollToken() {
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&device_code=" + _auth_state.device_code +
                    "&scope=user-read-playback-state"
                    "&grant_type=urn:ietf:params:oauth:grant-type:device_code";

  _request = _http.request({.method = http::Method::POST,
                            .url = kAuthTokenUrl,
                            .headers = {{kContentType, kXWWWFormUrlencoded}},
                            .body = data},
                           {.post_to = _main_scheduler, .callback = [this](auto response) {
                              onPollTokenResponse(std::move(response));
                            }});
}

void AuthenticatorPresenterImpl::onPollTokenResponse(http::Response response) {
  if (response.status / 100 == 5) {
    std::cerr << "failed to get auth_code or error" << std::endl;
    return scheduleAuthRetry();
  }

  TokenData data;
  parseTokenData(_jq, response.body, data);
  if (!data.error.empty()) {
    std::cerr << "auth_code error: " << data.error << std::endl;
    return data.error == "authorization_pending" ? scheduleNextPollToken() : scheduleAuthRetry();
  }

  std::cerr << "access_token: " << std::string_view(data.access_token).substr(0, 8) << ", "
            << "refresh_token: " << std::string_view(data.refresh_token).substr(0, 8) << std::endl;

  if (auto presenter_callback = std::exchange(_presenter_callback, {})) {
    presenter_callback();
  }

  // don't access this after token_callback
  _token_callback(std::move(data.access_token), std::move(data.refresh_token));
}

void AuthenticatorPresenterImpl::scheduleNextPollToken() {
  _work = _main_scheduler.schedule(
      [this] {
        if (std::chrono::system_clock::now() >= _auth_state.expiry) {
          return scheduleAuthRetry();
        }
        pollToken();
      },
      {.delay = _auth_state.interval});
}

}  // namespace spotify
