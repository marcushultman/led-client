#include "spotify_client.h"

#include <array>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "credentials.h"
#include "font/font.h"
#include "spotiled.h"

namespace {

const auto kAuthDeviceCodeUrl = "https://accounts.spotify.com/api/device/code";
const auto kAuthTokenUrl = "https://accounts.spotify.com/api/token";
const auto kPlayerUrl = "https://api.spotify.com/v1/me/player";

const auto kContentTypeXWWWFormUrlencoded = "Content-Type: application/x-www-form-urlencoded";
const auto kAuthorizationBearer = "Authorization: Bearer ";

struct NowPlaying {
  std::string track_id;
  std::string context_href;
  std::string context;
  std::string title;
  std::string artist;
  std::string image;
  std::string uri;
  std::chrono::milliseconds progress;
  std::chrono::milliseconds duration;
};

bool curl_perform_and_check(CURL *curl, long &status) {
  return curl_easy_perform(curl) || curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status) ||
         status >= 400;
}

bool curl_perform_and_check(CURL *curl) {
  long status{0};
  return curl_perform_and_check(curl, status);
}

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

#if 0

struct AuthCodeData {
  std::string error;
  std::string auth_code;
};

AuthCodeData parseAuthCodeData(jq_state *jq, const std::string &buffer) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq, ".error, .auth_code");
  jq_start(jq, input, 0);
  return {nextStr(jq), nextStr(jq)};
}

#endif

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
  return {nextStr(jq), nextStr(jq), "",         nextStr(jq), nextStr(jq),
          nextStr(jq), nextStr(jq), nextMs(jq), nextMs(jq)};
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

int pixel(int col, int offset) { return 16 * col + 8 + offset; }

}  // namespace

class SpotifyClientImpl final : public SpotifyClient {
 public:
  SpotifyClientImpl(CURL *curl, jq_state *jq, uint8_t brightness, bool verbose);

  int run() final;

 private:
  enum PollResult {
    kPollSuccess = 0,
    kPollWait,
    kPollError,
  };

  // spotify device code auth flow
  void authenticate();
  bool authenticateCode(const std::string &device_code,
                        const std::string &user_code,
                        const std::chrono::seconds &expires_in,
                        const std::chrono::seconds &interval);
  bool fetchToken(const std::string &device_code,
                  const std::string &user_code,
                  const std::chrono::seconds &expires_in,
                  const std::chrono::seconds &interval);
  PollResult pollToken(const std::string &device_code);
  bool refreshToken();

  void loop();

  bool fetchNowPlaying(bool retry);
  void fetchContext(const std::string &url);
  void fetchScannable(const std::string &uri);

  void displayString(const std::chrono::milliseconds &elapsed, const std::string &s);
  void displayScannable();

  CURL *_curl = nullptr;
  jq_state *_jq = nullptr;
  uint8_t _logo_brightness = 16;
  uint8_t _brightness = 8;
  bool _verbose = false;
  std::unique_ptr<SpotiLED> _led;

  std::string _access_token;
  std::string _refresh_token;
  bool _has_played = false;

  NowPlaying _now_playing;
  std::array<uint8_t, 23> _lengths0, _lengths1;
};

std::unique_ptr<SpotifyClient> SpotifyClient::create(CURL *curl,
                                                     jq_state *jq,
                                                     uint8_t brightness,
                                                     bool verbose) {
  return std::make_unique<SpotifyClientImpl>(curl, jq, brightness, verbose);
}

SpotifyClientImpl::SpotifyClientImpl(CURL *curl, jq_state *jq, uint8_t brightness, bool verbose)
    : _curl{curl},
      _jq{jq},
      _led{SpotiLED::create()},
      _logo_brightness{std::min<uint8_t>(2 * brightness, uint8_t(32))},
      _brightness{brightness},
      _verbose{verbose} {
  std::cout << "Using logo brightness: " << int(_logo_brightness)
            << ", brightness: " << int(_brightness) << std::endl;
}

int SpotifyClientImpl::run() {
  for (;;) {
    authenticate();
    loop();
  }
  return 0;
}

void SpotifyClientImpl::authenticate() {
  curl_easy_reset(_curl);
  const auto header = curl_slist_append(nullptr, kContentTypeXWWWFormUrlencoded);
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&scope=user-read-playback-state"
                    "&description=Spotify-LED";

  for (;;) {
    curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(_curl, CURLOPT_URL, kAuthDeviceCodeUrl);

    std::string buffer;
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, bufferString);

    if (curl_perform_and_check(_curl)) {
      std::cerr << "failed to get device_code" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds{5});
      std::cerr << "retrying..." << std::endl;
      continue;
    }
    auto res = parseDeviceFlowData(_jq, buffer);
    std::cerr << "url: " << res.verification_url_prefilled << std::endl;

    if (authenticateCode(res.device_code, res.user_code, res.expires_in, res.interval)) {
      return;
    }
    std::cerr << "failed to authenticate. retrying..." << std::endl;
  }
}

bool SpotifyClientImpl::authenticateCode(const std::string &device_code,
                                         const std::string &user_code,
                                         const std::chrono::seconds &expires_in,
                                         const std::chrono::seconds &interval) {
  using namespace std::chrono_literals;
  displayString(0ms, user_code);
  return fetchToken(device_code, user_code, expires_in, interval);
}

bool SpotifyClientImpl::fetchToken(const std::string &device_code,
                                   const std::string &user_code,
                                   const std::chrono::seconds &expires_in,
                                   const std::chrono::seconds &interval) {
  using namespace std::chrono_literals;
  using std::chrono::system_clock;

  auto elapsed = 0ms;
  auto expiry = system_clock::now() + expires_in;
  while (auto err = pollToken(device_code)) {
    if (err == kPollError) {
      return false;
    }
    for (auto end = elapsed + interval; elapsed < end; elapsed += 200ms) {
      std::this_thread::sleep_for(200ms);
      displayString(elapsed, user_code);
    }
    if (system_clock::now() >= expiry) {
      return false;
    }
  }
  return true;
}

SpotifyClientImpl::PollResult SpotifyClientImpl::pollToken(const std::string &device_code) {
  const auto header = curl_slist_append(nullptr, kContentTypeXWWWFormUrlencoded);
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&device_code=" + device_code +
                    "&scope=user-read-playback-state"
                    "&grant_type=urn:ietf:params:oauth:grant-type:device_code";

  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, header);
  curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(_curl, CURLOPT_URL, kAuthTokenUrl);

  std::string buffer;
  curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, bufferString);

  if (curl_easy_perform(_curl)) {
    std::cerr << "failed to get auth_code or error" << std::endl;
    return kPollError;
  }
  auto res = parseTokenData(_jq, buffer);
  if (!res.error.empty()) {
    std::cerr << "auth_code error: " << res.error << std::endl;
    return res.error == "authorization_pending" ? kPollWait : kPollError;
  }

  std::cerr << "access_token: " << res.access_token << std::endl;
  std::cerr << "refresh_token: " << res.refresh_token << std::endl;

  _access_token = res.access_token;
  _refresh_token = res.refresh_token;

  return kPollSuccess;
}

bool SpotifyClientImpl::refreshToken() {
  const auto header = curl_slist_append(nullptr, kContentTypeXWWWFormUrlencoded);
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&client_secret=" + credentials::kClientSecret +
                    "&refresh_token=" + _refresh_token + "&grant_type=refresh_token";

  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, header);
  curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(_curl, CURLOPT_URL, kAuthTokenUrl);

  std::string buffer;
  curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, bufferString);

  if (curl_perform_and_check(_curl)) {
    std::cerr << "failed to refresh token" << std::endl;
    return false;
  }
  auto res = parseTokenData(_jq, buffer);
  std::cerr << "access_token: " << res.access_token << std::endl;

  _access_token = res.access_token;

  return true;
}

void SpotifyClientImpl::loop() {
  for (;;) {
    if (fetchNowPlaying(true)) {
      displayScannable();
    }
    if (_has_played && _now_playing.track_id.empty()) {
      _access_token = {};
      _refresh_token = {};
      _has_played = false;
      break;
    }
    for (auto i = 0; i < 5; ++i) {
      std::this_thread::sleep_for(std::chrono::seconds{1});
      _now_playing.progress += std::chrono::seconds{1};
    }
  }
}

bool SpotifyClientImpl::fetchNowPlaying(bool retry) {
  curl_easy_reset(_curl);
  const auto auth_header = kAuthorizationBearer + _access_token;
  const auto header = curl_slist_append(nullptr, auth_header.c_str());

  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, header);
  curl_easy_setopt(_curl, CURLOPT_URL, kPlayerUrl);

  std::string buffer;
  curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, bufferString);

  long status{0};
  if (curl_perform_and_check(_curl, status)) {
    if (!retry) {
      std::cerr << "failed to get player state" << std::endl;
    }
    return retry && refreshToken() && fetchNowPlaying(false);
  }
  if (status == 204) {
    if (!_now_playing.track_id.empty()) {
      std::cerr << "nothing is playing" << std::endl;
      _now_playing = {};
    }
    return true;
  }
  _has_played = true;
  auto now_playing = parseNowPlaying(_jq, buffer, _verbose);

  if (now_playing.track_id == _now_playing.track_id) {
    _now_playing.progress = now_playing.progress;
    return true;
  }
  _now_playing = now_playing;

  fetchContext(_now_playing.context_href);
  fetchScannable(_now_playing.uri);

  std::cerr << "context: " << _now_playing.context << "\n";
  std::cerr << "title: " << _now_playing.title << "\n";
  std::cerr << "artist: " << _now_playing.artist << "\n";
  std::cerr << "image: " << _now_playing.image << "\n";
  std::cerr << "uri: " << _now_playing.uri << "\n";
  std::cerr << "duration: " << _now_playing.duration.count() << std::endl;

  return true;
}

void SpotifyClientImpl::fetchContext(const std::string &url) {
  if (url.empty()) {
    return;
  }
  const auto auth_header = kAuthorizationBearer + _access_token;
  const auto header = curl_slist_append(nullptr, auth_header.c_str());

  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, header);
  curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());

  std::string buffer;
  curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, bufferString);

  if (curl_perform_and_check(_curl)) {
    std::cerr << "failed to fetch context" << std::endl;
    return;
  }
  _now_playing.context = parseContext(_jq, buffer);
}

void SpotifyClientImpl::fetchScannable(const std::string &uri) {
  curl_easy_reset(_curl);
  const auto url = credentials::kScannablesCdnUrl + uri + "?format=svg";

  curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());

  std::string buffer;
  curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, bufferString);

  long status = 0;
  if (curl_perform_and_check(_curl, status)) {
    std::cerr << "failed to fetch scannable id, status: " << status << std::endl;
    _lengths0.fill(0);
    _lengths1.fill(0);
    return;
  }

  auto sv = std::string_view(buffer);
  sv.remove_prefix(sv.find("<rect"));
  sv.remove_prefix(sv.find("/>"));

  auto map = std::map<int, int>{
      {11, 1}, {44, 1}, {18, 2}, {41, 2}, {25, 3}, {37, 3}, {32, 4}, {34, 4},
      {39, 5}, {30, 5}, {46, 6}, {27, 6}, {53, 7}, {23, 7}, {60, 8}, {20, 8},
  };

  for (auto i = 0; i < 23; ++i) {
    sv.remove_prefix(sv.find(" y="));
    sv.remove_prefix(4);
    _lengths0[i] = map[std::atoi(sv.data())];
    sv.remove_prefix(sv.find("height="));
    sv.remove_prefix(8);
    _lengths1[i] = map[std::atoi(sv.data())];
  }
}

void SpotifyClientImpl::displayString(const std::chrono::milliseconds &elapsed,
                                      const std::string &s) {
  const auto kScrollSpeed = 0.005;

  _led->clear();
  _led->setLogo(Color{_logo_brightness, _logo_brightness, _logo_brightness});

  static auto _text = font::TextPage::create();
  static auto _presenter =
      RollingPresenter::create(*_led, Direction::kHorizontal, _brightness, _logo_brightness);

  _presenter->HACK_setElapsed(elapsed);

  _text->setText(s);
  _presenter->present(*_text);
}

void SpotifyClientImpl::displayScannable() {
  _led->clear();
  _led->setLogo({_logo_brightness, _logo_brightness, _logo_brightness});

  for (auto col = 0; col < 23; ++col) {
    auto start = _lengths0[col];
    auto end = _lengths1[col];
    for (auto y = start; y < end; ++y) {
      _led->set({.x = col, .y = y}, {_brightness, _brightness, _brightness});
    }
  }
  _led->show();
}
