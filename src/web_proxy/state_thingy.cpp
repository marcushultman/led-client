#include "state_thingy.h"

#include <charconv>
#include <fstream>
#include <iostream>
#include <map>
#include <utility>

#include "encoding/base64.h"
#include "storage/string_set.h"
#include "uri/uri.h"

extern "C" {
#include <jq.h>
}

namespace web_proxy {
namespace {

using namespace std::chrono_literals;

constexpr auto kStatesFilename = "states";

constexpr auto kInitialRetryBackoff = 5s;

auto toNumber(jv jv_val) {
  auto number = jv_get_kind(jv_val) == JV_KIND_NUMBER ? jv_number_value(jv_val) : 0;
  jv_free(jv_val);
  return number;
}

struct HumanReadableDuration {
  HumanReadableDuration(std::chrono::milliseconds d) : _d{d} {}
  friend auto &operator<<(std::ostream &os, const HumanReadableDuration &lhs) {
    return os << (lhs._d < 1min ? duration_cast<std::chrono::seconds>(lhs._d).count()
                                : duration_cast<std::chrono::minutes>(lhs._d).count())
              << (lhs._d < 1min ? "s" : "min");
  }
  std::chrono::milliseconds _d;
};

}  // namespace

StateThingy::StateThingy(async::Scheduler &main_scheduler,
                         RequestUpdate request_update,
                         std::unique_ptr<render::Renderer> renderer,
                         Callbacks callbacks)
    : _main_scheduler(main_scheduler),
      _request_update(std::move(request_update)),
      _renderer(std::move(renderer)),
      _state_callbacks{std::move(callbacks)},
      _load_work{_main_scheduler.schedule([this] { loadStates(); })},
      _save_work{
          _main_scheduler.schedule([this] { saveStates(); }, {.delay = 10s, .period = 1min})} {
  _renderer->add([this](auto &led, auto elapsed) { return onRender(led, elapsed); });
}

StateThingy::~StateThingy() { saveStates(); }

void StateThingy::loadStates() {
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
    onState(id, state.data);
    _request_update(std::move(id), state);
  }
  _snapshot = set;
}

void StateThingy::saveStates() {
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

const std::unordered_map<std::string, State> &StateThingy::states() { return _states; }

std::optional<http::Response> StateThingy::handleRequest(const http::Request &req) {
  if (req.method == http::Method::GET) {
    return handleGetRequest(req);
  }
  if (req.method == http::Method::POST) {  // todo: more factors?
    return handlePostRequest(req);
  }
  return {};
}

std::optional<http::Response> StateThingy::handleGetRequest(const http::Request &req) {
  auto url = uri::Uri(req.url);
  if (auto it = _states.find(std::string(url.path.full)); it != _states.end()) {
    return it->second.data;
  }
  return {};
}

void StateThingy::updateState(std::string id) { _request_update(id, _states[id]); }

http::Response StateThingy::handlePostRequest(const http::Request &req) {
  auto url = uri::Uri(req.url);
  auto id = std::string(url.path.full);
  auto &state = _states[id];

  auto it = req.headers.find("content-type");
  auto content_type = it != req.headers.end() ? it->second : std::string_view();

  if (content_type == "application/json") {
    if (!handleStateUpdate(req.body)) {
      std::cerr << id << ": update failed (explicit)" << std::endl;
      return 400;
    }
  } else if (!req.body.empty() && content_type == "application/x-www-form-urlencoded") {
    _request_update(id + "?" + req.body, state);
  } else {
    _request_update({url.path.full.begin(), url.end()}, state);
  }
  return 204;
}

bool StateThingy::handleStateUpdate(const std::string &json) {
  auto jv_dict = jv_parse(json.c_str());

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

      if (auto data =
              jv_get_kind(jv_data) == JV_KIND_STRING ? jv_string_value(jv_data) : std::string();
          data != state.data) {
        state.data = std::move(data);
        onState(id, state.data);
      }
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

        display.prio = static_cast<present::Prio>(
            toNumber(jv_object_get(jv_copy(jv_display), jv_string("prio"))));
        display.width = toNumber(jv_object_get(jv_copy(jv_display), jv_string("width")));
        display.height = toNumber(jv_object_get(jv_copy(jv_display), jv_string("height")));
        display.xscroll = toNumber(jv_object_get(jv_copy(jv_display), jv_string("xscroll")));
        display.wave = toNumber(jv_object_get(jv_copy(jv_display), jv_string("wave")));

        if (_displaying && (display.prio > _displaying->prio || _displaying == &*state.display)) {
          _displaying = nullptr;
        }

        state.display = display;

        if (!_displaying) {
          _displaying = &*state.display;
          _renderer->notify();
        }

      } else if (state.display) {
        if (_displaying == &*state.display) {
          _displaying = nullptr;
          _renderer->notify();
        }
        state.display.reset();

        if (auto *id = findNextToDisplay()) {
          updateState(*id);
        }
      }
      jv_free(jv_display);

      // timeout & poll
      auto jv_timeout = jv_object_get(jv_copy(jv_val), jv_string("timeout"));
      auto jv_poll = jv_object_get(jv_copy(jv_val), jv_string("poll"));
      if (jv_get_kind(jv_timeout) == JV_KIND_NUMBER) {
        auto delay = std::chrono::milliseconds{static_cast<int64_t>(jv_number_value(jv_timeout))};
        std::cout << id << ": updated, timeout in " << HumanReadableDuration(delay) << std::endl;
        state.work = _main_scheduler.schedule(
            [this, id, &state] {
              if (_displaying == &*state.display) {
                _displaying = nullptr;
                _renderer->notify();
              }
              _states.erase(id);
              std::cout << id << ": erased" << std::endl;

              if (auto *id = findNextToDisplay()) {
                updateState(*id);
              }
            },
            {.delay = delay});
      } else if (jv_get_kind(jv_poll) == JV_KIND_NUMBER) {
        auto delay = std::chrono::milliseconds{static_cast<int64_t>(jv_number_value(jv_poll))};
        std::cout << id << ": updated, poll in " << HumanReadableDuration(delay) << std::endl;
        state.work = _main_scheduler.schedule(
            [this, id, &state] { _request_update(std::move(id), state); }, {.delay = delay});
      } else {
        std::cout << id << ": updated" << std::endl;
        state.work = {};
      }
      jv_free(jv_poll);
      jv_free(jv_timeout);

    } else if (kind == JV_KIND_NULL) {
      if (auto it = _states.find(id); it != _states.end()) {
        if (_displaying == &*it->second.display) {
          _displaying = nullptr;
          _renderer->notify();
        }
        _states.erase(it);
        std::cout << id << ": erased" << std::endl;

        if (auto *id = findNextToDisplay()) {
          updateState(*id);
        }
      }
    }
    jv_free(jv_val);
  }
  jv_free(jv_dict);

  return true;
}

void StateThingy::onServiceResponse(http::Response res, std::string id, State &state) {
  if (res.status == 204) {
    return;
  }
  if (res.status == 200) {
    state.retry_backoff = {};
    if (handleStateUpdate(res.body)) {
      // todo: do we always get 200?
      return;
    }
  }

  if (res.status == 404) {
    std::cerr << id << ": update failed (status " << res.status << "), erasing" << std::endl;
    _states.erase(id);
    return;
  }
  if (res.status == 429 || res.status == 503) {
    if (auto it = res.headers.find("retry-after"); it != res.headers.end()) {
      auto &str = it->second;
      if (int s = 0; std::from_chars(str.data(), str.data() + str.size(), s).ec == std::errc{}) {
        state.retry_backoff = std::chrono::seconds(s);
      }
    }
  } else {
    state.retry_backoff = state.retry_backoff.count()
                              ? std::min<std::chrono::milliseconds>(2 * state.retry_backoff, 10min)
                              : kInitialRetryBackoff;
  }

  std::cerr << id << ": update failed (status " << res.status << "), retrying in "
            << duration_cast<std::chrono::seconds>(state.retry_backoff).count() << "s" << std::endl;
  state.work = _main_scheduler.schedule(
      [this, id = std::move(id), &state] { _request_update(std::move(id), state); },
      {.delay = state.retry_backoff});
}

const std::string *StateThingy::findNextToDisplay() const {
  std::map<present::Prio, const std::string *> id_by_display_prio;
  for (auto &[id, state] : _states) {
    if (auto &display = state.display) {
      id_by_display_prio[display->prio] = &id;
    }
  }
  return id_by_display_prio.empty() ? nullptr : id_by_display_prio.rbegin()->second;
}

std::chrono::milliseconds StateThingy::onRender(render::LED &led,
                                                std::chrono::milliseconds elapsed) {
  if (_displaying) {
    _displaying->onRenderPass(led, elapsed);

    if (_displaying->xscroll || _displaying->wave) {
      return 100ms;
    }
  }
  return 1h;
}

void StateThingy::onState(std::string_view id, std::string_view data) {
  for (auto [it, end] = _state_callbacks.equal_range(std::string(id)); it != end; ++it) {
    it->second(data);
  }
}

}  // namespace web_proxy
