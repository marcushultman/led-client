#include "spotify_client.h"

#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "apps/settings/brightness_provider.h"
#include "credentials.h"
#include "font/font.h"
#include "present/rolling_presenter.h"
#include "util/spotiled/spotiled.h"
#include "util/storage/string_set.h"

extern "C" {
#include <jq.h>
}

namespace spotify {
namespace {

const auto kAuthDeviceCodeUrl = "https://accounts.spotify.com/api/device/code";
const auto kAuthTokenUrl = "https://accounts.spotify.com/api/token";
const auto kPlayerUrl = "https://api.spotify.com/v1/me/player";

const auto kContentType = "content-type";
const auto kXWWWFormUrlencoded = "application/x-www-form-urlencoded";
const auto kAuthorization = "authorization";

void nextStr(jq_state *jq, std::string &value) {
  const auto jv = jq_next(jq);
  if (jv_get_kind(jv) != JV_KIND_STRING) {
    jv_free(jv);
    value.clear();
    return;
  }
  value = jv_string_value(jv);
  jv_free(jv);
}

double nextNumber(jq_state *jq) {
  const auto jv = jq_next(jq);
  if (jv_get_kind(jv) != JV_KIND_NUMBER) {
    jv_free(jv);
    return 0;
  }
  const auto val = jv_number_value(jv);
  jv_free(jv);
  return val;
}

void nextSeconds(jq_state *jq, std::chrono::seconds &value) {
  using sec = std::chrono::seconds;
  value = sec{static_cast<sec::rep>(nextNumber(jq))};
}

void nextMs(jq_state *jq, std::chrono::milliseconds &value) {
  using ms = std::chrono::milliseconds;
  value = ms{static_cast<ms::rep>(nextNumber(jq))};
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

void parseNowPlaying(jq_state *jq, const std::string &buffer, NowPlaying &now_playing) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq,
             ".item.id,"
             ".context.href,"
             ".item.name,"
             "(.item.artists | map(.name) | join(\", \")),"
             "(.item.album.images | min_by(.width)).url,"
             ".item.uri,"
             ".progress_ms,"
             ".item.duration_ms");
  jq_start(jq, input, 0);
  nextStr(jq, now_playing.track_id);
  nextStr(jq, now_playing.context_href);
  nextStr(jq, now_playing.title);
  nextStr(jq, now_playing.artist);
  nextStr(jq, now_playing.image);
  nextStr(jq, now_playing.uri);
  nextMs(jq, now_playing.progress);
  nextMs(jq, now_playing.duration);
}

#if 0

void parseContext(jq_state *jq, const std::string &buffer, std::string &value) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq, ".name");
  jq_start(jq, input, 0);
  nextStr(jq, value);
}

#endif

constexpr auto kTokensFilename = "spotify_token";

std::vector<std::unique_ptr<NowPlaying>> loadTokens() {
  std::vector<std::unique_ptr<NowPlaying>> now_playing;

  std::unordered_set<std::string> set;
  storage::loadSet(set, std::ifstream(kTokensFilename));
  for (auto token : set) {
    auto split = token.find('\n');
    now_playing.push_back(std::make_unique<NowPlaying>());
    now_playing.back()->access_token = token.substr(0, split);
    now_playing.back()->refresh_token = token.substr(split + 1);
  }
  return now_playing;
}

void saveTokens(const std::vector<std::unique_ptr<NowPlaying>> &now_playing) {
  std::unordered_set<std::string> set;
  for (auto &now_playing : now_playing) {
    set.insert(now_playing->access_token + "\n" + now_playing->refresh_token);
  }
  auto out = std::ofstream(kTokensFilename);
  storage::saveSet(set, out);
}

}  // namespace

std::chrono::milliseconds requestBackoff(size_t num_request) {
  using namespace std::chrono_literals;
  return num_request > 0 ? 30s * std::min(1 << (num_request - 1), 128) : 0s;
}

class NowPlayingServiceImpl final : public NowPlayingService {
 public:
  NowPlayingServiceImpl(
      async::Scheduler &main_scheduler, http::Http &http, jq_state *jq, bool verbose, OnPlaying);
  ~NowPlayingServiceImpl();

  void add(std::string access_token, std::string refresh_token) final {
    _now_playing.push_back(std::make_unique<NowPlaying>());
    _now_playing.back()->access_token = std::move(access_token);
    _now_playing.back()->refresh_token = std::move(refresh_token);
    saveTokens(_now_playing);
    scheduleFetchNowPlaying(*_now_playing.back());
  }

  const NowPlaying *getSomePlaying() const final {
    for (auto &now_playing : _now_playing) {
      if (now_playing->status == 200) {
        return now_playing.get();
      }
    }
    return nullptr;
  }

 private:
  enum PollResult {
    kPollSuccess = 0,
    kPollWait,
    kPollError,
  };

  void scheduleFetchNowPlaying(NowPlaying &now_playing);

  void refreshToken(NowPlaying &now_playing);
  void onRefreshTokenResponse(NowPlaying &now_playing, http::Response response);

  void fetchNowPlaying(bool allow_retry, NowPlaying &now_playing);
  void onNowPlayingResponse(bool allow_retry, NowPlaying &now_playing, http::Response response);

  // void fetchContext(const std::string &access_token, const std::string &url);
  // void onContext(const std::string &access_token, http::Response response);

  void fetchScannable(bool started_playing, NowPlaying &now_playing);
  void onScannable(bool started_playing, NowPlaying &now_playing, http::Response response);

  void renderScannable(bool started_playing, NowPlaying &now_playing);

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  jq_state *_jq = nullptr;
  bool _verbose = false;
  OnPlaying _on_playing;

  std::vector<std::unique_ptr<NowPlaying>> _now_playing;
};

std::unique_ptr<NowPlayingService> NowPlayingService::create(async::Scheduler &main_scheduler,
                                                             http::Http &http,
                                                             bool verbose,
                                                             OnPlaying on_playing) {
  auto jq = jq_init();
  if (!jq) {
    return nullptr;
  }
  return std::make_unique<NowPlayingServiceImpl>(main_scheduler, http, jq, verbose,
                                                 std::move(on_playing));
}

NowPlayingServiceImpl::NowPlayingServiceImpl(async::Scheduler &main_scheduler,
                                             http::Http &http,
                                             jq_state *jq,
                                             bool verbose,
                                             OnPlaying on_playing)
    : _main_scheduler{main_scheduler},
      _http{http},
      _jq{jq},
      _verbose{verbose},
      _on_playing{on_playing},
      _now_playing{loadTokens()} {
  printf("%lu tokens loaded\n", _now_playing.size());
  for (auto &now_playing : _now_playing) {
    scheduleFetchNowPlaying(*now_playing);
  }
}

NowPlayingServiceImpl::~NowPlayingServiceImpl() { jq_teardown(&_jq); }

void NowPlayingServiceImpl::scheduleFetchNowPlaying(NowPlaying &now_playing) {
  std::chrono::milliseconds delay;

  using namespace std::chrono_literals;
  if (now_playing.status == 0) {
    delay = 0s;
  } else if (now_playing.status == 200) {
    delay = 5s;
  } else {
    // Backoff for 204, 4xx, 5xx
    delay = requestBackoff(now_playing.num_request);
    auto token = std::string_view(now_playing.access_token).substr(0, 8);
    printf("[%.*s] retry in %llds\n", int(token.size()), token.data(),
           std::chrono::duration_cast<std::chrono::seconds>(delay).count());
  }
  now_playing.work = _main_scheduler.schedule(
      [this, &now_playing] { fetchNowPlaying(true, now_playing); }, {.delay = delay});
}

void NowPlayingServiceImpl::refreshToken(NowPlaying &now_playing) {
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&client_secret=" + credentials::kClientSecret +
                    "&refresh_token=" + now_playing.refresh_token + "&grant_type=refresh_token";

  now_playing.request =
      _http.request({.method = http::Method::POST,
                     .url = kAuthTokenUrl,
                     .headers = {{kContentType, kXWWWFormUrlencoded}},
                     .body = data},
                    {.post_to = _main_scheduler, .callback = [this, &now_playing](auto response) {
                       onRefreshTokenResponse(now_playing, std::move(response));
                     }});
}

void NowPlayingServiceImpl::onRefreshTokenResponse(NowPlaying &now_playing,
                                                   http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "failed to refresh token: " << response.body << std::endl;
    std::erase_if(_now_playing, [&](auto &ptr) { return ptr.get() == &now_playing; });
    saveTokens(_now_playing);
    return;
  }

  TokenData data;
  parseTokenData(_jq, response.body, data);
  std::cerr << "refreshed "
            << "access_token: " << std::string_view(data.access_token).substr(0, 8) << ", "
            << "refresh_token: " << std::string_view(data.refresh_token).substr(0, 8) << std::endl;

  now_playing.access_token = std::move(data.access_token);
  now_playing.refresh_token = std::move(data.refresh_token);
  saveTokens(_now_playing);

  fetchNowPlaying(false, now_playing);
}

void NowPlayingServiceImpl::fetchNowPlaying(bool allow_retry, NowPlaying &now_playing) {
  auto token = std::string_view(now_playing.access_token).substr(0, 8);
  printf("[%.*s] fetch NowPlaying\n", int(token.size()), token.data());

  now_playing.num_request++;
  now_playing.request = _http.request(
      {.url = kPlayerUrl, .headers = {{kAuthorization, "Bearer " + now_playing.access_token}}},
      {.post_to = _main_scheduler, .callback = [this, allow_retry, &now_playing](auto response) {
         onNowPlayingResponse(allow_retry, now_playing, std::move(response));
       }});
}

void NowPlayingServiceImpl::onNowPlayingResponse(bool allow_retry,
                                                 NowPlaying &now_playing,
                                                 http::Response response) {
  now_playing.status = response.status;

  if (response.status / 100 != 2) {
    if (!allow_retry) {
      std::cerr << "failed to get player state" << std::endl;
      return scheduleFetchNowPlaying(now_playing);
    }
    now_playing.num_request--;
    return refreshToken(now_playing);
  }

  if (response.status == 204) {
    std::cerr << "nothing is playing" << std::endl;
    now_playing.track_id.clear();
    return scheduleFetchNowPlaying(now_playing);
  }
  assert(response.status == 200);

  // signal that this just started playing
  auto started_playing = now_playing.status != response.status;

  now_playing.num_request = 0;

  auto track_id = now_playing.track_id;

  parseNowPlaying(_jq, response.body, now_playing);

  if (track_id == now_playing.track_id) {
    return renderScannable(started_playing, now_playing);
  }

  // fetchContext(_now_playing.context_href);
  fetchScannable(started_playing, now_playing);
}

#if 0

void NowPlayingServiceImpl::fetchContext(const std::string &access_token, const std::string &url) {
  if (url.empty()) {
    return fetchScannable(access_token, _now_playing.uri);
  }

  _request = _http.request(http::RequestInit{
      .url = url,
      .headers = {{"authorization", "Bearer " + access_token}},
      .post_to = _main_scheduler,
      .callback = [this,
                   access_token](auto response) { onContext(access_token, std::move(response)); },
  });
}

void NowPlayingServiceImpl::onContext(const std::string &access_token, http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "failed to fetch context" << std::endl;
    return fetchNowPlaying(true);
  }
  _now_playing.context = parseContext(_jq, response.body);

  fetchScannable(access_token, _now_playing.uri);
}

#endif

void NowPlayingServiceImpl::fetchScannable(bool started_playing, NowPlaying &now_playing) {
  now_playing.request =
      _http.request({.url = credentials::kScannablesCdnUrl + now_playing.uri + "?format=svg"},
                    {.post_to = _main_scheduler,
                     .callback = [this, started_playing, &now_playing](auto response) {
                       onScannable(started_playing, now_playing, std::move(response));
                     }});
}

void NowPlayingServiceImpl::onScannable(bool started_playing,
                                        NowPlaying &now_playing,
                                        http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "failed to fetch scannable id, status: " << response.status << std::endl;
    now_playing.lengths0.fill(0);
    now_playing.lengths1.fill(0);
    return renderScannable(started_playing, now_playing);
  }

  auto sv = std::string_view(response.body);
  sv.remove_prefix(sv.find("<rect"));
  sv.remove_prefix(sv.find("/>"));

  auto map = std::map<int, int>{
      {11, 1}, {44, 1}, {18, 2}, {41, 2}, {25, 3}, {37, 3}, {32, 4}, {34, 4},
      {39, 5}, {30, 5}, {46, 6}, {27, 6}, {53, 7}, {23, 7}, {60, 8}, {20, 8},
  };

  for (auto i = 0; i < 23; ++i) {
    sv.remove_prefix(sv.find(" y="));
    sv.remove_prefix(4);
    now_playing.lengths0[i] = map[std::atoi(sv.data())];
    sv.remove_prefix(sv.find("height="));
    sv.remove_prefix(8);
    now_playing.lengths1[i] = map[std::atoi(sv.data())];
  }

  renderScannable(started_playing, now_playing);
}

void NowPlayingServiceImpl::renderScannable(bool started_playing, NowPlaying &now_playing) {
  std::cerr << "context: " << now_playing.context << "\n";
  std::cerr << "title: " << now_playing.title << "\n";
  std::cerr << "artist: " << now_playing.artist << "\n";
  std::cerr << "image: " << now_playing.image << "\n";
  std::cerr << "uri: " << now_playing.uri << "\n";
  std::cerr << "duration: " << now_playing.duration.count() << std::endl;

  if (started_playing) {
    _on_playing(now_playing);
  }

  scheduleFetchNowPlaying(now_playing);
}

// AuthenticatorPresenter

class AuthenticatorPresenterImpl final : public AuthenticatorPresenter, present::Presentable {
 public:
  AuthenticatorPresenterImpl(async::Scheduler &main_scheduler,
                             http::Http &http,
                             SpotiLED &led,
                             present::PresenterQueue &presenter,
                             settings::BrightnessProvider &brightness,
                             jq_state *jq,
                             bool verbose,
                             AccessTokenCallback token_callback)
      : _main_scheduler{main_scheduler},
        _led{led},
        _http{http},
        _presenter{presenter},
        _brightness{brightness},
        _jq{jq},
        _token_callback{std::move(token_callback)} {
    _presenter.add(*this);
    authenticate();
  }
  ~AuthenticatorPresenterImpl() { jq_teardown(&_jq); }

  void start(SpotiLED &led, present::Callback) final;
  void stop() final;

 private:
  void authenticate();
  void scheduleAuthRetry(std::chrono::milliseconds delay = {});

  void onAuthResponse(http::Response response);
  void pollToken();
  void onPollTokenResponse(http::Response response);
  void scheduleNextPollToken();

  async::Scheduler &_main_scheduler;
  SpotiLED &_led;
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
  std::unique_ptr<RollingPresenter> _text_presenter;

  http::Lifetime _request;
  async::Lifetime _work;
};

std::unique_ptr<AuthenticatorPresenter> AuthenticatorPresenter::create(
    async::Scheduler &main_scheduler,
    http::Http &http,
    SpotiLED &led,
    present::PresenterQueue &presenter,
    settings::BrightnessProvider &brightness,
    bool verbose,
    AccessTokenCallback callback) {
  auto jq = jq_init();
  if (!jq) {
    return nullptr;
  }
  return std::make_unique<AuthenticatorPresenterImpl>(main_scheduler, http, led, presenter,
                                                      brightness, jq, verbose, callback);
}

void AuthenticatorPresenterImpl::start(SpotiLED &led, present::Callback callback) {
  _text_presenter.reset();
  _presenter_callback = callback;

  led.add([this](auto &led, auto elapsed) {
    using namespace std::chrono_literals;
    if (!_presenter_callback) {
      return 0ms;
    }

    if (!_auth_state.user_code.empty() && !_text_presenter) {
      renderRolling(_led, _brightness, elapsed, *_text);
    }

    return 200ms;
  });
}
void AuthenticatorPresenterImpl::stop() {
  _text_presenter.reset();
  _request.reset();
}

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

// NowPlayingPresenter

class NowPlayingPresenterImpl final : public NowPlayingPresenter, present::Presentable {
 public:
  NowPlayingPresenterImpl(present::PresenterQueue &presenter,
                          settings::BrightnessProvider &brightness,
                          const NowPlaying &now_playing)
      : _presenter{presenter}, _brightness{brightness}, _now_playing{now_playing} {
    _presenter.add(*this);
  }
  ~NowPlayingPresenterImpl() { _presenter.erase(*this); }

  void start(SpotiLED &led, present::Callback) {
    _alive = std::make_shared<bool>(true);
    led.add([this, sentinel = std::weak_ptr<void>(_alive)](auto &led, auto) {
      using namespace std::chrono_literals;
      if (sentinel.expired()) {
        return 0s;
      }
      displayScannable(led);
      return 5s;
    });
  }
  void stop() { _alive.reset(); }

 private:
  void displayScannable(SpotiLED &led);

  present::PresenterQueue &_presenter;
  settings::BrightnessProvider &_brightness;
  const NowPlaying &_now_playing;
  std::shared_ptr<bool> _alive;
};

std::unique_ptr<NowPlayingPresenter> NowPlayingPresenter::create(
    present::PresenterQueue &presenter,
    settings::BrightnessProvider &brightness,
    const NowPlaying &now_playing) {
  return std::make_unique<NowPlayingPresenterImpl>(presenter, brightness, now_playing);
}

void NowPlayingPresenterImpl::displayScannable(SpotiLED &led) {
  led.setLogo(_brightness.logoBrightness());
  auto brightness = _brightness.brightness();

  for (auto col = 0; col < 23; ++col) {
    auto start = 8 - _now_playing.lengths0[col];
    auto end = 8 + _now_playing.lengths1[col];
    for (auto y = start; y < end; ++y) {
      led.set({.x = col, .y = y}, brightness);
    }
  }
}

}  // namespace spotify
