#include "now_playing_service.h"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <map>

#include "credentials.h"
#include "jq_util.h"

extern "C" {
#include <jq.h>
}

namespace spotify {
namespace {

const auto kAuthTokenUrl = "https://accounts.spotify.com/api/token";
const auto kPlayerUrl = "https://api.spotify.com/v1/me/player?additional_types=track,episode";
const auto kAudioFeaturesUrl = std::string("https://api.spotify.com/v1/audio-features/");

const auto kContentType = "content-type";
const auto kXWWWFormUrlencoded = "application/x-www-form-urlencoded";
const auto kAuthorization = "authorization";

bool nextMs(jq_state *jq, std::chrono::milliseconds &value) {
  double s = 0.0;
  auto ok = nextNumber(jq, s);
  using ms = std::chrono::milliseconds;
  value = ms{static_cast<ms::rep>(s)};
  return ok;
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

bool parseTokenData(jq_state *jq, const std::string &buffer, TokenData &data) {
  const auto input = jv_parse(buffer.c_str());
  if (!jv_is_valid(input)) {
    return false;
  }
  jq_compile(jq, ".error, .access_token, .refresh_token");
  jq_start(jq, input, 0);
  return nextStr(jq, data.error) && nextStr(jq, data.access_token) &&
         nextStr(jq, data.refresh_token);
}

bool parseNowPlaying(jq_state *jq, const std::string &buffer, NowPlaying &now_playing) {
  const auto input = jv_parse(buffer.c_str());
  if (!jv_is_valid(input)) {
    return false;
  }
  jq_compile(
      jq,
      ".item.id,"
      ".context.href,"
      ".item.name,"
      "(if .item.artists then .item.artists | map(.name) | join(\", \") else .item.show.name end),"
      "(.item.album.images // .item.images | min_by(.width)).url,"
      ".item.uri,"
      ".item.id,"
      ".progress_ms,"
      ".item.duration_ms,"
      ".is_playing");
  jq_start(jq, input, 0);
  return nextStr(jq, now_playing.track_id) && nextStr(jq, now_playing.context_href) &&
         nextStr(jq, now_playing.title) && nextStr(jq, now_playing.artist) &&
         nextStr(jq, now_playing.image) && nextStr(jq, now_playing.uri) &&
         nextStr(jq, now_playing.id) && nextMs(jq, now_playing.progress) &&
         nextMs(jq, now_playing.duration) && nextBool(jq, now_playing.is_playing);
}

bool parseAudioFeatures(jq_state *jq, const std::string &buffer, double &bpm) {
  const auto input = jv_parse(buffer.c_str());
  if (!jv_is_valid(input)) {
    return false;
  }
  jq_compile(jq, ".tempo");
  jq_start(jq, input, 0);
  return nextNumber(jq, bpm);
}

#if 0

void parseContext(jq_state *jq, const std::string &buffer, std::string &value) {
  const auto input = jv_parse(buffer.c_str());
  jq_compile(jq, ".name");
  jq_start(jq, input, 0);
  nextStr(jq, value);
}

#endif

bool isLikelyToPlay() {
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  auto &lt = *std::localtime(&now);
  auto wday = lt.tm_wday + 6 % 7;  // days since Monday [0-6]
  auto hour = lt.tm_hour;

  return wday < 5 ? (hour >= 6 && hour <= 9) || (hour > 15 && hour <= 22)  // Mon - Fri
                  : (hour >= 8 && hour <= 23);                             // Weekend
}

}  // namespace

// backoff 10s -> ~5min
std::chrono::milliseconds requestBackoff(size_t num_request) {
  using namespace std::chrono_literals;
  auto max_factor = isLikelyToPlay() ? 6 : 32;
  return 10s * int(std::min<size_t>(1 << (std::max<size_t>(num_request, 1) - 1), max_factor));
}

class NowPlayingServiceImpl final : public NowPlayingService {
 public:
  NowPlayingServiceImpl(async::Scheduler &main_scheduler,
                        http::Http &http,
                        jq_state *jq,
                        bool verbose,
                        std::string access_token,
                        std::string refresh_token,
                        Callbacks &);
  ~NowPlayingServiceImpl();

  const std::string &accessToken() const final { return _access_token; }
  const std::string &refreshToken() const final { return _refresh_token; }

  const NowPlaying *getIfPlaying() const final {
    if (_now_playing.status == 200 && _now_playing.is_playing) {
      return &_now_playing;
    }
    return nullptr;
  }

 private:
  void scheduleFetchNowPlaying();
  void onStopped(bool was_playing);
  void onRateLimited(http::Response);

  void refreshToken();
  void onRefreshTokenResponse(http::Response response);

  void fetchNowPlaying(bool allow_retry);
  void onNowPlayingResponse(bool allow_retry, http::Response response);

  // void fetchContext(const std::string &access_token, const std::string &url);
  // void onContext(const std::string &access_token, http::Response response);

  void fetchScannable(bool started_playing);
  void onScannable(http::Response response);

  void renderScannable(bool started_playing);

  async::Scheduler &_main_scheduler;
  http::Http &_http;
  jq_state *_jq = nullptr;
  bool _verbose = false;

  std::string _access_token;
  std::string _refresh_token;
  NowPlaying _now_playing;

  Callbacks &_callbacks;
};

std::unique_ptr<NowPlayingService> NowPlayingService::create(async::Scheduler &main_scheduler,
                                                             http::Http &http,
                                                             bool verbose,
                                                             std::string access_token,
                                                             std::string refresh_token,
                                                             Callbacks &callbacks) {
  auto jq = jq_init();
  if (!jq) {
    return nullptr;
  }
  return std::make_unique<NowPlayingServiceImpl>(main_scheduler, http, jq, verbose,
                                                 std::move(access_token), std::move(refresh_token),
                                                 callbacks);
}

NowPlayingServiceImpl::NowPlayingServiceImpl(async::Scheduler &main_scheduler,
                                             http::Http &http,
                                             jq_state *jq,
                                             bool verbose,
                                             std::string access_token,
                                             std::string refresh_token,
                                             Callbacks &callbacks)
    : _main_scheduler{main_scheduler},
      _http{http},
      _jq{jq},
      _verbose{verbose},
      _access_token{std::move(access_token)},
      _refresh_token{std::move(refresh_token)},
      _callbacks{callbacks} {
  scheduleFetchNowPlaying();
}

NowPlayingServiceImpl::~NowPlayingServiceImpl() { jq_teardown(&_jq); }

void NowPlayingServiceImpl::scheduleFetchNowPlaying() {
  std::chrono::milliseconds delay;

  using namespace std::chrono_literals;
  if (_now_playing.status == 0) {
    delay = 0s;
  } else if (_now_playing.status == 200) {
    delay = 5s;
  } else {
    // Backoff for 204, 4xx, 5xx
    delay = requestBackoff(_now_playing.num_request);
    std::cout << "[" << std::string_view(_access_token).substr(0, 8) << "]"
              << " retry in " << std::chrono::duration_cast<std::chrono::seconds>(delay).count()
              << "s (" << _now_playing.num_request << ")" << std::endl;
  }
  _now_playing.work = _main_scheduler.schedule([this] { fetchNowPlaying(true); }, {.delay = delay});
}

void NowPlayingServiceImpl::refreshToken() {
  const auto data = std::string{"client_id="} + credentials::kClientId +
                    "&client_secret=" + credentials::kClientSecret +
                    "&refresh_token=" + _refresh_token + "&grant_type=refresh_token";

  _now_playing.request = _http.request(
      {.method = http::Method::POST,
       .url = kAuthTokenUrl,
       .headers = {{kContentType, kXWWWFormUrlencoded}},
       .body = data},
      {.post_to = _main_scheduler,
       .on_response = [this](auto response) { onRefreshTokenResponse(std::move(response)); }});
}

void NowPlayingServiceImpl::onRefreshTokenResponse(http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "[" << std::string_view(_access_token).substr(0, 8) << "] " << response.status
              << " failed to refresh token: " << response.body << std::endl;
    _callbacks.onLogout(*this);
    return;
  }

  TokenData data;
  if (!parseTokenData(_jq, response.body, data)) {
    std::cerr << "[" << std::string_view(_access_token).substr(0, 8) << "] " << response.status
              << " invalid JSON: " << response.body << std::endl;
    _callbacks.onLogout(*this);
    return;
  }
  std::cout << "[" << std::string_view(_access_token).substr(0, 8) << "] refreshed "
            << "access_token: " << std::string_view(data.access_token).substr(0, 8) << ", "
            << "refresh_token: " << std::string_view(data.refresh_token).substr(0, 8) << std::endl;

  _access_token = std::move(data.access_token);
  _refresh_token = std::move(data.refresh_token);
  _callbacks.onTokenUpdate(*this);

  fetchNowPlaying(false);
}

void NowPlayingServiceImpl::fetchNowPlaying(bool allow_retry) {
  std::cout << "[" << std::string_view(_access_token).substr(0, 8) << "]"
            << " fetch NowPlaying" << std::endl;

  _now_playing.num_request = std::min<size_t>(_now_playing.num_request + 1, 32);
  _now_playing.request =
      _http.request({.url = kPlayerUrl, .headers = {{kAuthorization, "Bearer " + _access_token}}},
                    {.post_to = _main_scheduler, .on_response = [this, allow_retry](auto response) {
                       onNowPlayingResponse(allow_retry, std::move(response));
                     }});
}

void NowPlayingServiceImpl::onNowPlayingResponse(bool allow_retry, http::Response response) {
  auto prev_status = std::exchange(_now_playing.status, response.status);
  auto was_playing = prev_status == 200 && _now_playing.is_playing;

  if (response.status / 100 != 2) {
    if (response.status == 429) {
      return onRateLimited(std::move(response));
    }
    if (!allow_retry) {
      std::cerr << "[" << std::string_view(_access_token).substr(0, 8) << "] " << response.status
                << " failed to get player state" << std::endl;
      return scheduleFetchNowPlaying();
    }
    _now_playing.num_request--;
    return refreshToken();
  }

  if (response.status == 204) {
    return onStopped(was_playing);
  }
  assert(response.status == 200);

  _now_playing.num_request = 0;

  auto track_id = _now_playing.track_id;

  if (!parseNowPlaying(_jq, response.body, _now_playing)) {
    std::cerr << "[" << std::string_view(_access_token).substr(0, 8) << "] " << response.status
              << " invalid JSON: " << response.body << std::endl;
    return scheduleFetchNowPlaying();
  }

  if (!_now_playing.is_playing) {
    return onStopped(was_playing);
  }

  if (track_id == _now_playing.track_id) {
    return renderScannable(false);
  }

  // fetchContext(_now_playing.context_href);
  fetchScannable(!was_playing);
}

void NowPlayingServiceImpl::onStopped(bool was_playing) {
  std::cout << "[" << std::string_view(_access_token).substr(0, 8) << "] "
            << "nothing is playing" << std::endl;
  _now_playing.track_id.clear();
  if (was_playing) {
    _callbacks.onStopped(*this);
  }
  return scheduleFetchNowPlaying();
}

void NowPlayingServiceImpl::onRateLimited(http::Response response) {
  using namespace std::chrono;
  auto delay =
      std::clamp<milliseconds>(seconds(std::stoi(response.headers["retry-after"])), 60s, 24h);
  std::cerr << "[" << std::string_view(_access_token).substr(0, 8) << "] " << response.status
            << " rate limited, retry in "
            << std::chrono::duration_cast<std::chrono::seconds>(delay).count() << "s" << std::endl;
  _now_playing.work = _main_scheduler.schedule([this] { fetchNowPlaying(true); }, {.delay = delay});
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

void NowPlayingServiceImpl::fetchScannable(bool started_playing) {
  struct Composite {
    Composite(unsigned remaining) : remaining{remaining} {}
    void operator()(NowPlayingServiceImpl &self, bool started_playing) {
      if (--remaining == 0) self.renderScannable(started_playing);
    }
    unsigned remaining = 0;
  };
  auto remaining = std::make_shared<Composite>(2);
  auto lifetimes = std::vector<http::Lifetime>{
      _http.request({.url = credentials::kScannablesCdnUrl + _now_playing.uri + "?format=svg"},
                    {.post_to = _main_scheduler,
                     .on_response =
                         [this, started_playing, remaining](auto response) {
                           onScannable(std::move(response));
                           (*remaining)(*this, started_playing);
                         }}),
      _http.request({.url = kAudioFeaturesUrl + _now_playing.id,
                     .headers = {{kAuthorization, "Bearer " + _access_token}}},
                    {.post_to = _main_scheduler,
                     .on_response =
                         [this, started_playing, remaining](auto res) {
                           parseAudioFeatures(_jq, res.body, _now_playing.bpm);
                           (*remaining)(*this, started_playing);
                         }}),
  };
  _now_playing.request = std::make_shared<decltype(lifetimes)>(std::move(lifetimes));
}

void NowPlayingServiceImpl::onScannable(http::Response response) {
  if (response.status / 100 != 2) {
    std::cerr << "[" << std::string_view(_access_token).substr(0, 8) << "] " << response.status
              << " failed to fetch scannable id" << std::endl;
    _now_playing.lengths0.fill(0);
    _now_playing.lengths1.fill(0);
    return;
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
    _now_playing.lengths0[i] = map[std::atoi(sv.data())];
    sv.remove_prefix(sv.find("height="));
    sv.remove_prefix(8);
    _now_playing.lengths1[i] = map[std::atoi(sv.data())];
  }
}

void NowPlayingServiceImpl::renderScannable(bool started_playing) {
  std::cout << "[" << std::string_view(_access_token).substr(0, 8) << "] "
            << "Spotify: " << _now_playing.title << " - " << _now_playing.artist << " ("
            << _now_playing.uri << ") " << _now_playing.context << std::endl;

  _callbacks.onNewTrack(*this, _now_playing);

  if (started_playing) {
    _callbacks.onPlaying(*this, _now_playing);
  }

  scheduleFetchNowPlaying();
}

}  // namespace spotify
