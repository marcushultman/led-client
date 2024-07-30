#include "web_proxy.h"

#include <fstream>
#include <iostream>
#include <unordered_set>

#include "display.h"
#include "util/encoding/base64.h"
#include "util/storage/string_set.h"
#include "util/url/url.h"

extern "C" {
#include <jq.h>
}

namespace web_proxy {
namespace {

using namespace std::chrono_literals;

constexpr auto kDefaultBaseUrl = "https://spotiled.deno.dev";
constexpr auto kHostHeader = "host";
constexpr auto kStatesFilename = "states";

std::string makeJSON(const char *key, jv value) {
  auto jv = jv_dump_string(jv_object_set(jv_object(), jv_string(key), value), 0);
  std::string ret = jv_string_value(jv);
  jv_free(jv);
  return ret;
}

}  // namespace

struct State {
  std::string data;
  std::optional<Display> display;
  async::Lifetime work;
};

struct StateThingy final {
  using UpdateState = std::function<void(std::string id, State &, bool retry_allowed)>;

  StateThingy(async::Scheduler &main_scheduler,
              UpdateState update_state,
              present::PresenterQueue &presenter)
      : _main_scheduler(main_scheduler),
        _update_state(std::move(update_state)),
        _presenter(presenter),
        _load_work{_main_scheduler.schedule([this] { loadStates(); })},
        _save_work{
            _main_scheduler.schedule([this] { saveStates(); }, {.delay = 10s, .period = 1min})} {}
  ~StateThingy() { saveStates(); }

  void loadStates() {
    std::unordered_set<std::string> set;
    storage::loadSet(set, std::ifstream(kStatesFilename));

    set.empty() ? (std::cout << "No states" << std::endl)
                : (std::cout << "Loading " << set.size() << " states" << std::endl);

    for (auto &entry : set) {
      auto split = entry.find('\n');
      auto id = entry.substr(0, split);
      auto &state = _states[id];
      state.data = entry.substr(split + 1);
      std::cout << id << ": loaded (" << state.data << ")" << std::endl;
      _update_state(std::move(id), state, true);
    }
    _snapshot = set;
  }

  void saveStates() {
    std::unordered_set<std::string> set;
    for (auto &[id, state] : _states) {
      set.insert(id + "\n" + state.data);
    }

    if (set != _snapshot) {
      auto out = std::ofstream(kStatesFilename);
      storage::saveSet(set, out);
      _snapshot = set;
      _states.empty() ? (std::cout << "states cleared" << std::endl)
                      : (std::cout << _states.size() << " states saved" << std::endl);
    }
  }

  const std::unordered_map<std::string, State> &states() { return _states; }

  void updateState(url::Url url) {
    auto id = std::string{url.path.full.begin(), url.end()};
    auto &state = _states[id.substr(0, url.path.full.size())];
    _update_state(id, state, true);
  }

  bool handleServiceResponse(const http::Response &res) {
    if (res.status == 204) {
      return true;
    } else if (res.status != 200) {
      // todo: do we always get 200?
      return false;
    }
    auto jv_dict = jv_parse(res.body.c_str());

    if (jv_get_kind(jv_dict) != JV_KIND_OBJECT) {
      jv_free(jv_dict);
      return false;
    }

    // todo: split up this massive function?
    jv_object_foreach(jv_dict, jv_id, jv_val) {
      auto id = std::string(jv_string_value(jv_id));
      jv_free(jv_id);

      if (auto kind = jv_get_kind(jv_val); kind == JV_KIND_OBJECT) {
        auto &state = _states[id];

        // data
        auto jv_data = jv_object_get(jv_copy(jv_val), jv_string("data"));
        state.data =
            jv_get_kind(jv_data) == JV_KIND_STRING ? jv_string_value(jv_data) : std::string();
        jv_free(jv_data);

        // Display
        auto jv_display = jv_object_get(jv_copy(jv_val), jv_string("display"));
        if (jv_get_kind(jv_display) == JV_KIND_OBJECT) {
          auto display = Display();

          auto jv_logo = jv_object_get(jv_copy(jv_display), jv_string("logo"));
          if (jv_get_kind(jv_logo) == JV_KIND_ARRAY) {
            display.logo = Color{
                static_cast<uint8_t>(jv_number_value(jv_array_get(jv_copy(jv_logo), 0))),
                static_cast<uint8_t>(jv_number_value(jv_array_get(jv_copy(jv_logo), 1))),
                static_cast<uint8_t>(jv_number_value(jv_array_get(jv_copy(jv_logo), 2))),
            };
          } else if (jv_get_kind(jv_logo) == JV_KIND_STRING) {
            const auto bytes = encoding::base64::decode(jv_string_value(jv_logo));
            display.logo = Color{
                static_cast<uint8_t>(bytes[0]),
                static_cast<uint8_t>(bytes[1]),
                static_cast<uint8_t>(bytes[2]),
            };
          }
          jv_free(jv_logo);

          auto jv_bytes = jv_object_get(jv_copy(jv_display), jv_string("bytes"));
          if (jv_get_kind(jv_bytes) == JV_KIND_ARRAY) {
            display.bytes.resize(jv_array_length(jv_copy(jv_bytes)));
            for (auto i = 0; i < display.bytes.size(); ++i) {
              display.bytes[i] =
                  static_cast<uint8_t>(jv_number_value(jv_array_get(jv_copy(jv_bytes), i)));
            }
          } else if (jv_get_kind(jv_bytes) == JV_KIND_STRING) {
            display.bytes = encoding::base64::decode(jv_string_value(jv_bytes));
          }
          jv_free(jv_bytes);

          auto jv_prio = jv_object_get(jv_copy(jv_display), jv_string("prio"));
          if (jv_get_kind(jv_prio) == JV_KIND_NUMBER) {
            display.prio = static_cast<present::Prio>(jv_number_value(jv_prio));
          }
          jv_free(jv_prio);

          if (state.display && state.display->prio != display.prio) {
            _presenter.erase(*state.display);
            state.display = {};
          }

          if (!std::exchange(state.display, std::move(display))) {
            _presenter.add(*state.display, {.prio = state.display->prio});
          } else {
            _presenter.notify();
          }
        } else if (state.display) {
          _presenter.erase(*state.display);
          state.display = {};
        }
        jv_free(jv_display);

        // Behavior
        auto jv_behavior = jv_object_get(jv_copy(jv_val), jv_string("behavior"));
        if (jv_get_kind(jv_behavior) == JV_KIND_OBJECT) {
          auto jv_poll = jv_object_get(jv_copy(jv_behavior), jv_string("poll"));
          if (jv_get_kind(jv_poll) == JV_KIND_NUMBER) {
            auto delay = std::chrono::milliseconds{static_cast<int64_t>(jv_number_value(jv_poll))};
            std::cout << id << ": updated, poll in "
                      << (delay < 1min
                              ? std::chrono::duration_cast<std::chrono::seconds>(delay).count()
                              : std::chrono::duration_cast<std::chrono::minutes>(delay).count())
                      << (delay < 1min ? "s" : "min") << std::endl;
            state.work = _main_scheduler.schedule(
                [this, id, &state] { _update_state(std::move(id), state, true); },
                {.delay = delay});
          }
          jv_free(jv_poll);
        } else {
          std::cout << id << ": updated" << std::endl;
          state.work = {};
        }
        jv_free(jv_behavior);
      } else if (kind == JV_KIND_NULL) {
        if (auto it = _states.find(id); it != _states.end()) {
          if (it->second.display) {
            _presenter.erase(*it->second.display);
          }
          _states.erase(it);
          std::cout << id << ": erased" << std::endl;
        }
      }
      jv_free(jv_val);
    }
    jv_free(jv_dict);

    return true;
  }

  void onServiceResponse(const http::Response &res,
                         std::string id,
                         State &state,
                         bool retry_allowed) {
    if (handleServiceResponse(res)) {
      return;
    }

    if (retry_allowed) {
      std::cerr << id << ": update failed (status " << res.status << "), retrying in 5s"
                << std::endl;
      state.work = _main_scheduler.schedule(
          [this, id = std::move(id), &state] { _update_state(std::move(id), state, false); },
          {.delay = 5s});
    } else {
      std::cerr << id << ": update failed (status " << res.status << "), suspending" << std::endl;
    }
  }

 private:
  async::Scheduler &_main_scheduler;
  UpdateState _update_state;
  present::PresenterQueue &_presenter;
  std::unordered_map<std::string, State> _states;
  async::Lifetime _load_work, _save_work;
  std::unordered_set<std::string> _snapshot;
};

//

WebProxy::WebProxy(async::Scheduler &main_scheduler,
                   http::Http &http,
                   spotiled::BrightnessProvider &brightness,
                   present::PresenterQueue &presenter,
                   spotify::SpotifyService &spotify,
                   std::string base_url)
    : _main_scheduler{main_scheduler},
      _http{http},
      _brightness{brightness},
      _spotify{spotify},
      _base_url{base_url.empty() ? kDefaultBaseUrl : std::move(base_url)},
      _base_host{url::Url(_base_url).host},
      _state_thingy{std::make_unique<StateThingy>(
          _main_scheduler,
          [this](auto id, auto &state, auto retry_allowed) {
            updateState(std::move(id), state, retry_allowed);
          },
          presenter)} {}

WebProxy::~WebProxy() = default;

http::Lifetime WebProxy::handleRequest(http::Request req,
                                       http::RequestOptions::OnResponse on_response,
                                       http::RequestOptions::OnBytes on_bytes) {
  if (req.url[0] == '*') {
    req.url = _base_url;
  } else if (req.url[0] == '/') {
    req.url = _base_url + req.url;
  }

  if (req.method == http::Method::POST) {  // todo: more factors?
    _state_thingy->updateState(url::Url(req.url));
    return _main_scheduler.schedule([on_response] { on_response(204); });
  }

  if (auto it = req.headers.find(kHostHeader); it != req.headers.end()) {
    it->second = _base_host;
  }

  auto sp = jv_object();
  sp = jv_object_set(sp, jv_string("isAuthenticating"),
                     _spotify.isAuthenticating() ? jv_true() : jv_false());
  sp = jv_object_set(sp, jv_string("tokens"), _spotify.getTokens());

  auto states = jv_object();
  for (auto &[id, state] : _state_thingy->states()) {
    states = jv_object_set(states, jv_string(id.c_str()), jv_string(state.data.c_str()));
  }

  auto jv = jv_object();
  jv = jv_object_set(jv, jv_string("brightness"), jv_number(_brightness.brightness()));
  jv = jv_object_set(jv, jv_string("hue"), jv_number(_brightness.hue()));
  jv = jv_object_set(jv, jv_string("spotify"), sp);
  jv = jv_object_set(jv, jv_string("states"), states);
  jv = jv_dump_string(jv, 0);
  req.headers["x-spotiled"] = encoding::base64::encode(jv_string_value(jv));
  jv_free(jv);

  return _http.request(
      std::move(req),
      {.post_to = _main_scheduler,
       .on_response = [this, on_response](auto res) { on_response(std::move(res)); },
       .on_bytes = std::move(on_bytes)});
}

void WebProxy::updateState(std::string id, State &state, bool retry_allowed) {
  auto url = _base_url + (id.starts_with('/') ? "" : "/") + std::string(id);
  state.work = _http.request(
      {.method = http::Method::POST,
       .url = std::move(url),
       .headers = {{"content-type", "application/json"}, {"accept-encoding", "identity"}},
       .body = makeJSON("data", jv_string(state.data.c_str()))},
      {.post_to = _main_scheduler,
       .on_response = [this, id = std::move(id), &state, retry_allowed](auto res) {
         _state_thingy->onServiceResponse(res, std::move(id), state, retry_allowed);
       }});
}

}  // namespace web_proxy
