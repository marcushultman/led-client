#include "spotify_client.h"

#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "credentials.h"
#include "font/font.h"
#include "storage/string_set.h"

extern "C" {
#include <jq.h>
}

namespace {

const auto kAuthDeviceCodeUrl = "https://accounts.spotify.com/api/device/code";
const auto kAuthTokenUrl = "https://accounts.spotify.com/api/token";
const auto kPlayerUrl = "https://api.spotify.com/v1/me/player";

const auto kContentType = "content-type";
const auto kXWWWFormUrlencoded = "application/x-www-form-urlencoded";
const auto kAuthorization = "authorization";

struct NowPlaying {
  int status = 0;
  std::string track_id;
  std::string context_href;
  std::string context;
  std::string title;
  std::string artist;
  std::string image;
  std::string uri;
  std::chrono::milliseconds progress;
  std::chrono::milliseconds duration;
  std::array<uint8_t, 23> lengths0, lengths1;
};

std::string nextStr(jq_state *jq) {
  const auto jv = jq_next(jq);
  if (jv_get_kind(jv) != JV_KIND_STRING) {
    jv_free(jv);
    return "";
  }
  const auto val = std::string{jv_string_value(jv)};
  jv_free(jv);
  return val;
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

std::chrono::seconds nextSeconds(jq_state *jq) {
  using sec = std::chrono::seconds;
  return sec{static_cast<sec::rep>(nextNumber(jq))};
}

std::chrono::milliseconds nextMs(jq_state *jq) {
  using ms = std::chrono::milliseconds;
  return ms{static_cast<ms::rep>(nextNumber(jq))};
}

struct DeviceFlowData {
  std::string device_code;
  std::string user_code;
  std::chrono::seconds expires_in;
  std::string verification_url;
  std::string verification_url_prefilled;
  std::chrono::seconds interval;
};

DeviceFlowData parseDeviceFlowData(jq_state *jq, const std::string &buffer) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq,
             ".device_code, .user_code, .expires_in, .verification_url, "
             ".verification_url_prefilled, .interval");
  jq_start(jq, input, 0);
  return {nextStr(jq), nextStr(jq), nextSeconds(jq), nextStr(jq), nextStr(jq), nextSeconds(jq)};
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

TokenData parseTokenData(jq_state *jq, const std::string &buffer) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq, ".error, .access_token, .refresh_token");
  jq_start(jq, input, 0);
  return {nextStr(jq), nextStr(jq), nextStr(jq)};
}

NowPlaying parseNowPlaying(jq_state *jq, const std::string &buffer, bool verbose) {
  if (verbose) {
    std::cout << "parseNowPlaying: " << buffer.c_str() << std::endl;
  }
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
  return {0,           nextStr(jq), nextStr(jq), "",         nextStr(jq), nextStr(jq),
          nextStr(jq), nextStr(jq), nextMs(jq),  nextMs(jq), {},          {}};
}

std::string parseContext(jq_state *jq, const std::string &buffer) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq, ".name");
  jq_start(jq, input, 0);
  return nextStr(jq);
}

size_t bufferString(char *ptr, size_t size, size_t nmemb, void *obj) {
  size *= nmemb;
  static_cast<std::string *>(obj)->append(ptr, size);
  return size;
}

size_t bufferArray(char *ptr, size_t size, size_t nmemb, void *obj) {
  size *= nmemb;
  auto &v = *static_cast<std::vector<unsigned char> *>(obj);
  v.insert(v.end(), ptr, ptr + size);
  return size;
}

std::pair<int, int> makeRange(int col, int upper, int lower) {
  auto odd = col % 2 == 1;
  auto start = odd ? -upper : -lower;
  auto end = odd ? lower : upper;
  auto mid = 16 * col + 8;
  return {mid + start, mid + end};
}

constexpr auto kTokensFilename = "spotify_token";

std::unordered_map<std::string, std::string> loadTokens() {
  std::unordered_map<std::string, std::string> tokens;

  std::unordered_set<std::string> set;
  storage::loadSet(set, std::ifstream(kTokensFilename));
  for (auto token : set) {
    auto split = token.find('\n');
    tokens[token.substr(0, split)] = token.substr(split + 1);
  }
  return tokens;
}

void saveTokens(const std::unordered_map<std::string, std::string> &tokens) {
  std::unordered_set<std::string> set;
  for (auto &[access_token, refresh_token] : tokens) {
    set.insert(access_token + "\n" + refresh_token);
  }
  auto out = std::ofstream(kTokensFilename);
  storage::saveSet(set, out);
}

}  // namespace

class SpotifyClientImpl final : public SpotifyClient {
 public:
  SpotifyClientImpl(async::Scheduler &main_scheduler,
                    http::Http &http,
                    SpotiLED &led,
                    BrightnessProvider &brightness,
                    jq_state *jq,
                    bool verbose);
  ~SpotifyClientImpl();

 private:
  enum PollResult {
    kPollSuccess = 0,
    kPollWait,
    kPollError,
  };

  void entrypoint();
  void scheduleAuthRetry(std::chrono::milliseconds delay = {});

  // spotify device code auth flow
  void authenticate();
  void onAuthResponse(http::Response response);

  void fetchToken(const std::string &device_code,
                  const std::string &user_code,
                  const std::chrono::seconds &expires_in,
                  const std::chrono::seconds &interval);

  void pollToken();
  void onPollTokenResponse(http::Response response);
  void scheduleNextPollToken();

  void refreshToken(const std::string &refresh_token);
  void onRefreshTokenResponse(http::Response response);

  void fetchNowPlaying(bool allow_retry);
  void onNowPlayingResponse(bool allow_retry,
                            const std::string &access_token,
                            http::Response response);

  // void fetchContext(const std::string &url);
  // void onContext(http::Response response);

  void fetchScannable(const std::string &access_token, const std::string &uri);
  void onScannable(const std::string &access_token, http::Response response);

  void renderScannable(const NowPlaying &now_playing);
  void displayScannable(const NowPlaying &now_playing);

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  SpotiLED &_led;
  BrightnessProvider &_brightness;
  jq_state *_jq = nullptr;
  bool _verbose = false;

  std::unordered_map<std::string, std::string> _tokens;
  bool _has_played = false;

  std::unordered_map<std::string, NowPlaying> _now_playing;

  struct AuthState {
    std::chrono::system_clock::time_point expiry;
    std::chrono::milliseconds interval;
    std::string device_code;
    std::string user_code;
  };
  AuthState _auth_state;

  std::unique_ptr<font::TextPage> _text = font::TextPage::create();
  std::unique_ptr<RollingPresenter> _text_presenter;

  std::unique_ptr<http::Request> _request;
  async::Lifetime _work;
};

std::unique_ptr<SpotifyClient> SpotifyClient::create(async::Scheduler &main_scheduler,
                                                     http::Http &http,
                                                     SpotiLED &led,
                                                     BrightnessProvider &brightness,
                                                     bool verbose) {
  auto jq = jq_init();
  if (!jq) {
    return nullptr;
  }
  return std::make_unique<SpotifyClientImpl>(main_scheduler, http, led, brightness, jq, verbose);
}

SpotifyClientImpl::SpotifyClientImpl(async::Scheduler &main_scheduler,
                                     http::Http &http,
                                     SpotiLED &led,
                                     BrightnessProvider &brightness,
                                     jq_state *jq,
                                     bool verbose)
    : _main_scheduler{main_scheduler},
      _http{http},
      _led{led},
      _brightness{brightness},
      _jq{jq},
      _verbose{verbose} {
  std::cout << "Using logo brightness: " << int(_brightness.logoBrightness()[0])
            << ", brightness: " << int(_brightness.brightness()[0]) << std::endl;
  _work = _main_scheduler.schedule([this] { entrypoint(); }, {});
}

SpotifyClientImpl::~SpotifyClientImpl() { jq_teardown(&_jq); }

void SpotifyClientImpl::entrypoint() {
  _tokens = loadTokens();
  _tokens.empty() ? authenticate() : fetchNowPlaying(true);
}

void SpotifyClientImpl::scheduleAuthRetry(std::chrono::milliseconds delay) {
  _work = _main_scheduler.schedule(
      [this] {
        std::cerr << "failed to authenticate. retrying..." << std::endl;
        authenticate();
      },
      {.delay = delay});
}

void SpotifyClientImpl::authenticate() {
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&scope=user-read-playback-state"
                    "&description=Spotify-LED";

  _request = _http.request({
      .method = http::Method::POST,
      .url = kAuthDeviceCodeUrl,
      .headers = {{kContentType, kXWWWFormUrlencoded}},
      .body = data,
      .post_to = _main_scheduler,
      .callback = [this](auto response) { onAuthResponse(std::move(response)); },
  });
}

void SpotifyClientImpl::onAuthResponse(http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "failed to get device_code" << std::endl;
    scheduleAuthRetry(std::chrono::seconds{5});
    return;
  }

  auto data = parseDeviceFlowData(_jq, response.body);
  std::cerr << "url: " << data.verification_url_prefilled << std::endl;

  _text->setText(data.user_code);
  _text_presenter = RollingPresenter::create(_main_scheduler, _led, _brightness, *_text,
                                             Direction::kHorizontal, {});

  fetchToken(data.device_code, data.user_code, data.expires_in, data.interval);
}

void SpotifyClientImpl::fetchToken(const std::string &device_code,
                                   const std::string &user_code,
                                   const std::chrono::seconds &expires_in,
                                   const std::chrono::seconds &interval) {
  auto now = std::chrono::system_clock::now();
  _auth_state.expiry = now + expires_in;
  _auth_state.interval = interval;
  _auth_state.device_code = device_code;
  _auth_state.user_code = user_code;

  pollToken();
}

void SpotifyClientImpl::pollToken() {
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&device_code=" + _auth_state.device_code +
                    "&scope=user-read-playback-state"
                    "&grant_type=urn:ietf:params:oauth:grant-type:device_code";

  _request = _http.request({
      .method = http::Method::POST,
      .url = kAuthTokenUrl,
      .headers = {{kContentType, kXWWWFormUrlencoded}},
      .body = data,
      .post_to = _main_scheduler,
      .callback = [this](auto response) { onPollTokenResponse(std::move(response)); },
  });
}

void SpotifyClientImpl::onPollTokenResponse(http::Response response) {
  if (response.status / 100 == 5) {
    std::cerr << "failed to get auth_code or error" << std::endl;
    return scheduleAuthRetry();
  }

  auto data = parseTokenData(_jq, response.body);
  if (!data.error.empty()) {
    std::cerr << "auth_code error: " << data.error << std::endl;
    return data.error == "authorization_pending" ? scheduleNextPollToken() : scheduleAuthRetry();
  }

  _text_presenter.reset();

  std::cerr << "access_token: " << data.access_token << std::endl;
  std::cerr << "refresh_token: " << data.refresh_token << std::endl;

  _tokens[data.access_token] = data.refresh_token;
  saveTokens(_tokens);

  fetchNowPlaying(true);
}

void SpotifyClientImpl::scheduleNextPollToken() {
  _work = _main_scheduler.schedule(
      [this] {
        if (std::chrono::system_clock::now() >= _auth_state.expiry) {
          return scheduleAuthRetry();
        }
        pollToken();
      },
      {.delay = _auth_state.interval});
}

void SpotifyClientImpl::refreshToken(const std::string &refresh_token) {
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&client_secret=" + credentials::kClientSecret +
                    "&refresh_token=" + refresh_token + "&grant_type=refresh_token";

  _request = _http.request({
      .method = http::Method::POST,
      .url = kAuthTokenUrl,
      .headers = {{kContentType, kXWWWFormUrlencoded}},
      .body = data,
      .post_to = _main_scheduler,
      .callback = [this](auto response) { onRefreshTokenResponse(std::move(response)); },
  });
}

void SpotifyClientImpl::onRefreshTokenResponse(http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "failed to refresh token" << std::endl;
    return scheduleAuthRetry();
  }

  auto data = parseTokenData(_jq, response.body);
  std::cerr << "access_token: " << data.access_token << std::endl;

  _tokens[data.access_token] = data.refresh_token;
  saveTokens(_tokens);

  fetchNowPlaying(false);
}

void SpotifyClientImpl::fetchNowPlaying(bool allow_retry) {
  for (auto &[access_token, _] : _tokens) {
    if (_now_playing[access_token].status == 204) {
      continue;
    }
    _request = _http.request(http::RequestInit{
        .url = kPlayerUrl,
        .headers = {{kAuthorization, "Bearer " + access_token}},
        .post_to = _main_scheduler,
        .callback =
            [this, allow_retry, access_token = access_token](auto response) {
              onNowPlayingResponse(allow_retry, access_token, std::move(response));
            },
    });
    return;
  }
  // no one is playing
  authenticate();
}

void SpotifyClientImpl::onNowPlayingResponse(bool allow_retry,
                                             const std::string &access_token,
                                             http::Response response) {
  if (response.status / 100 != 2) {
    if (!allow_retry) {
      std::cerr << "failed to get player state" << std::endl;
      return scheduleAuthRetry();
    }
    return refreshToken(access_token);
  }

  auto &now_playing = _now_playing[access_token];

  if (response.status == 204) {
    if (!now_playing.track_id.empty()) {
      std::cerr << "nothing is playing" << std::endl;
    }
    now_playing = {};
    now_playing.status = response.status;
    return renderScannable(now_playing);
  }

  _has_played = true;
  auto new_now_playing = parseNowPlaying(_jq, response.body, _verbose);

  if (now_playing.track_id == new_now_playing.track_id) {
    now_playing.progress = new_now_playing.progress;
    return renderScannable(now_playing);
  }
  now_playing = new_now_playing;
  now_playing.status = response.status;

  // fetchContext(_now_playing.context_href);
  fetchScannable(access_token, now_playing.uri);
}

#if 0

void SpotifyClientImpl::fetchContext(const std::string &url) {
  if (url.empty()) {
    return fetchScannable(_now_playing.uri);
  }

  _request = _http.request(http::RequestInit{
      .url = url,
      .headers = {{"authorization", "Bearer " + _access_token}},
      .post_to = _main_scheduler,
      .callback = [this](auto response) { onContext(std::move(response)); },
  });
}

void SpotifyClientImpl::onContext(http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "failed to fetch context" << std::endl;
    return authenticate();
  }
  _now_playing.context = parseContext(_jq, response.body);

  fetchScannable(_now_playing.uri);
}

#endif

void SpotifyClientImpl::fetchScannable(const std::string &access_token, const std::string &uri) {
  _request = _http.request(http::RequestInit{
      .url = credentials::kScannablesCdnUrl + uri + "?format=svg",
      .post_to = _main_scheduler,
      .callback = [this,
                   access_token](auto response) { onScannable(access_token, std::move(response)); },
  });
}

void SpotifyClientImpl::onScannable(const std::string &access_token, http::Response response) {
  auto &now_playing = _now_playing[access_token];

  if (response.status / 100 != 2) {
    std::cerr << "failed to fetch scannable id, status: " << response.status << std::endl;
    now_playing.lengths0.fill(0);
    now_playing.lengths1.fill(0);
    return renderScannable(now_playing);
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

  renderScannable(now_playing);
}

void SpotifyClientImpl::renderScannable(const NowPlaying &now_playing) {
  if (_has_played && now_playing.track_id.empty()) {
    _led.clear();
    _has_played = false;
    return fetchNowPlaying(true);
  }

  std::cerr << "context: " << now_playing.context << "\n";
  std::cerr << "title: " << now_playing.title << "\n";
  std::cerr << "artist: " << now_playing.artist << "\n";
  std::cerr << "image: " << now_playing.image << "\n";
  std::cerr << "uri: " << now_playing.uri << "\n";
  std::cerr << "duration: " << now_playing.duration.count() << std::endl;

  displayScannable(now_playing);
  _work = _main_scheduler.schedule([this] { fetchNowPlaying(true); },
                                   {.delay = std::chrono::seconds{5}});
}

void SpotifyClientImpl::displayScannable(const NowPlaying &now_playing) {
  _led.clear();
  _led.setLogo(_brightness.logoBrightness());
  auto brightness = _brightness.brightness();

  for (auto col = 0; col < 23; ++col) {
    auto start = 8 - now_playing.lengths0[col];
    auto end = 8 + now_playing.lengths1[col];
    for (auto y = start; y < end; ++y) {
      _led.set({.x = col, .y = y}, brightness);
    }
  }
  _led.show();
}
