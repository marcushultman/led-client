#include "spotify_client.h"

#include <iostream>
#include <map>
#include <thread>

#include "alpha_map.h"
#include "apa102.h"
#include "credentials.h"

namespace {

const auto kAuthDeviceCodeUrl = "https://accounts.spotify.com/api/device/code";
const auto kAuthTokenUrl = "https://accounts.spotify.com/api/token";
const auto kPlayerUrl = "https://api.spotify.com/v1/me/player";

const auto kContentTypeXWWWFormUrlencoded = "Content-Type: application/x-www-form-urlencoded";
const auto kAuthorizationBearer = "Authorization: Bearer ";

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

SpotifyClient::SpotifyClient(CURL *curl, jq_state *jq, uint8_t brightness, bool verbose)
    : _curl{curl},
      _jq{jq},
      _led{apa102::createLED(19 + 16 * 23)},
      _logo_brightness{std::min<uint8_t>(2 * brightness, uint8_t(32))},
      _brightness{brightness},
      _verbose{verbose} {
  std::cout << "Using logo brightness: " << int(_logo_brightness)
            << ", brightness: " << int(_brightness) << std::endl;
}

int SpotifyClient::run() {
  for (;;) {
    authenticate();
    loop();
  }
  return 0;
}

void SpotifyClient::authenticate() {
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

bool SpotifyClient::authenticateCode(const std::string &device_code, const std::string &user_code,
                                     const std::chrono::seconds &expires_in,
                                     const std::chrono::seconds &interval) {
  using namespace std::chrono_literals;
  displayString(0ms, user_code);
  return fetchToken(device_code, user_code, expires_in, interval);
}

bool SpotifyClient::fetchToken(const std::string &device_code, const std::string &user_code,
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

SpotifyClient::PollResult SpotifyClient::pollToken(const std::string &device_code) {
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

bool SpotifyClient::refreshToken() {
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

void SpotifyClient::loop() {
  for (;;) {
    if (fetchNowPlaying(true)) {
      displayNPV();
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
      displayNPV();
    }
  }
}

bool SpotifyClient::fetchNowPlaying(bool retry) {
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

void SpotifyClient::fetchContext(const std::string &url) {
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

void SpotifyClient::fetchScannable(const std::string &uri) {
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

void SpotifyClient::displayString(const std::chrono::milliseconds &elapsed, const std::string &s) {
  const auto kScrollSpeed = 0.005;

  _led->clear();

  for (auto i = 0; i < 19; ++i) {
    _led->set(i, _logo_brightness, _logo_brightness, _logo_brightness);
  }

  auto offset = 23 - (static_cast<int>(kScrollSpeed * elapsed.count()) % (23 + 30));

  for (auto n = 0; n < s.size(); ++n) {
    auto it = kAlphaMap.find(s[n]);
    if (it == kAlphaMap.end()) {
      std::cerr << "invalid alpha-numeric char: " << s[n] << std::endl;
      continue;
    }
    auto &glyph = it->second;
    for (auto col = 0; col < glyph.size(); ++col) {
      auto pixel_col = offset + 5 * n + col;
      if (pixel_col < 0) {
        continue;
      }
      for (auto i : glyph[col]) {
        _led->set(19 + pixel(pixel_col, i), _brightness, _brightness, _brightness);
      }
    }
  }

  _led->show();
}

// clang-format off
// const auto ranges = std::vector<std::pair<int, int>>{
//   makeRange(0, 1, 1),
//   makeRange(1, 7, 7),
//   makeRange(2, 1, 1),
//   makeRange(3, 3, 3),
//   makeRange(4, 5, 5),
//   makeRange(5, 6, 6),
//   makeRange(6, 2, 2),
//   makeRange(7, 5, 5),
//   makeRange(8, 6, 6),
//   makeRange(9, 3, 3),
//   makeRange(10, 4, 4),
//   makeRange(11, 8, 8),
//   makeRange(12, 4, 4),
//   makeRange(13, 8, 8),
//   makeRange(14, 3, 3),
//   makeRange(15, 6, 6),
//   makeRange(16, 7, 7),
//   makeRange(17, 3, 3),
//   makeRange(18, 6, 6),
//   makeRange(19, 8, 8),
//   makeRange(20, 5, 5),
//   makeRange(21, 4, 4),
//   makeRange(22, 1, 1),
// };
// clang-format on

void SpotifyClient::displayNPV() {
  // for (auto &[start, end] : ranges) {
  //   for (auto i = start; i < end; ++i) {
  //     _led->set(i, 1, 1, 1);
  //   }
  // }
  // _led->show();

  /*
  const auto title_offset = (38 - _now_playing.title.substr(0, 38).size()) / 2;
  const auto artist_offset = (40 - _now_playing.artist.substr(0, 40).size()) /
  2;

  using namespace templates;
  std::ofstream file{_out_file, std::ofstream::binary};
  file.write(kNpv, kNpvContextOffset);
  file << " " << _now_playing.context.substr(0, 33).c_str();
  file.write(kNpv + kNpvContextOffset, kNpvImageOffset - kNpvContextOffset);
  for (auto line = kNpvImageLineBegin, i = 0; line < kNpvImageLineEnd; ++line,
  ++i) { file << "OL," << line << ",         " << _image->line(i) << "\n";
  }
  file.write(kNpv + kNpvImageOffset, kNpvTitleOffset - kNpvImageOffset);
  for (auto i = 0u; i < title_offset; ++i) {
    file << " ";
  }
  file << _now_playing.title.substr(0, 38).c_str();
  file.write(kNpv + kNpvTitleOffset, kNpvArtistOffset - kNpvTitleOffset);
  for (auto i = 0u; i < artist_offset; ++i) {
    file << " ";
  }
  file << _now_playing.artist.substr(0, 40).c_str();
  file.write(kNpv + kNpvArtistOffset, kNpvProgressOffset - kNpvArtistOffset);

  auto progress_label = std::string{"00:00"};
  auto duration_label = std::string{"00:00"};
  const auto progress_tm = chronoToTm(_now_playing.progress);
  const auto duration_tm = chronoToTm(_now_playing.duration);
  std::strftime(&progress_label[0], 6, "%M:%S", &progress_tm);
  std::strftime(&duration_label[0], 6, "%M:%S", &duration_tm);

  auto progress_width = kNpvProgressWidth *
  static_cast<double>(_now_playing.progress.count()) /
                        _now_playing.duration.count();
  file << "  " << progress_label << "\u001bW";
  for (auto i = 0; i < kNpvProgressWidth; ++i) {
    unsigned char out = 1 << 5;
    if (progress_width > i) {
      out |= 1 << 2 | 1 << 3;
    } else if (progress_width > i - 0.5) {
      out |= 1 << 2;
    }
    file << out;
  }
  file << "\u001bG" << duration_label;
  */
}

void SpotifyClient::displayScannable() {
  _led->clear();
  for (auto i = 0; i < 19; ++i) {
    _led->set(i, _logo_brightness, _logo_brightness, _logo_brightness);
  }
  for (auto col = 0; col < 23; ++col) {
    auto [start, end] = makeRange(col, _lengths0[col], _lengths1[col]);
    for (auto i = start; i < end; ++i) {
      _led->set(19 + i, _brightness, _brightness, _brightness);
    }
  }
  _led->show();
}