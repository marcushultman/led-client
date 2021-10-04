#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <curl/curl.h>
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

struct Scannable {
  uint64_t id0;
  uint64_t id1;
};

class SpotifyClient {
 private:
  enum AuthResult {
    kAuthSuccess = 0,
    kAuthError,
  };
  enum PollResult {
    kPollSuccess = 0,
    kPollWait,
    kPollError,
  };

 public:
  SpotifyClient(CURL *curl, jq_state *jq);

  int run();

 private:
  // spotify device code auth flow
  void authenticate();
  AuthResult authenticateCode(const std::string &device_code,
                              const std::string &user_code,
                              const std::string &verification_url,
                              const std::chrono::seconds &expires_in,
                              const std::chrono::seconds &interval);
  AuthResult getAuthCode(const std::string &device_code,
                         const std::string &user_code,
                         const std::string &verification_url,
                         const std::chrono::seconds &expires_in,
                         const std::chrono::seconds &interval,
                         std::string &auth_code);
  PollResult pollAuthCode(const std::string &device_code, std::string &auth_code);
  bool fetchTokens(const std::string &auth_code);
  bool refreshToken();

  // update loop
  void loop();

  bool fetchNowPlaying(bool retry);
  void fetchContext(const std::string &url);

 public:
  void fetchScannable(const std::string &uri);

  void displayCode(const std::chrono::milliseconds &elapsed,
                   const std::string &code,
                   const std::string &verification_url);
  void displayNPV();
  void displayScannable();

  CURL *_curl{nullptr};
  jq_state *_jq{nullptr};
  std::unique_ptr<apa102::LED> _led;

  std::string _access_token;
  std::string _refresh_token;
  bool _has_played{false};

  NowPlaying _now_playing;
  // Scannable _scannable{};
  std::array<uint8_t, 23> _lengths0, _lengths1;
};
