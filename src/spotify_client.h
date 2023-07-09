#pragma once

#include <curl/curl.h>

#include <array>
#include <chrono>
#include <memory>
#include <string>

#include "apa102.h"

extern "C" {
#include <jq.h>
}

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

class SpotifyClient {
 public:
  SpotifyClient(CURL *curl, jq_state *jq, uint8_t brightness, bool verbose);

  int run();

 private:
  enum PollResult {
    kPollSuccess = 0,
    kPollWait,
    kPollError,
  };

  // spotify device code auth flow
  void authenticate();
  bool authenticateCode(const std::string &device_code, const std::string &user_code,
                        const std::chrono::seconds &expires_in,
                        const std::chrono::seconds &interval);
  bool fetchToken(const std::string &device_code, const std::string &user_code,
                  const std::chrono::seconds &expires_in, const std::chrono::seconds &interval);
  PollResult pollToken(const std::string &device_code);
  bool refreshToken();

  void loop();

  bool fetchNowPlaying(bool retry);
  void fetchContext(const std::string &url);
  void fetchScannable(const std::string &uri);

  void displayString(const std::chrono::milliseconds &elapsed, const std::string &s);
  void displayNPV();
  void displayScannable();

  CURL *_curl{nullptr};
  jq_state *_jq{nullptr};
  uint8_t _brightness = 1;
  bool _verbose = false;
  std::unique_ptr<apa102::LED> _led;

  std::string _access_token;
  std::string _refresh_token;
  bool _has_played{false};

  NowPlaying _now_playing;
  std::array<uint8_t, 23> _lengths0, _lengths1;
};
