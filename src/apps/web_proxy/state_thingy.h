#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "async/scheduler.h"
#include "display.h"
#include "http/http.h"
#include "present/presenter.h"

namespace web_proxy {

struct State {
  std::string data;
  std::optional<Display> display;
  std::chrono::milliseconds retry_backoff = {};
  async::Lifetime work;
};

struct StateThingy final {
  using RequestUpdate = std::function<void(std::string id, State &)>;
  using StateCallback = std::function<void(std::string_view data, bool on_load)>;
  using Callbacks = std::unordered_multimap<std::string, StateCallback>;

  StateThingy(async::Scheduler &main_scheduler,
              RequestUpdate request_update,
              present::PresenterQueue &presenter,
              Callbacks);
  ~StateThingy();

  const std::unordered_map<std::string, State> &states();

  std::optional<http::Response> handleRequest(const http::Request &req);

  bool handleStateUpdate(const std::string &json);
  void onServiceResponse(http::Response res, std::string id, State &state);

 private:
  void loadStates();
  void saveStates();

  std::optional<http::Response> handleGetRequest(const http::Request &req);
  http::Response handlePostRequest(const http::Request &req);

  void onState(std::string_view id, std::string_view data, bool on_load);

  async::Scheduler &_main_scheduler;
  RequestUpdate _request_update;
  present::PresenterQueue &_presenter;
  std::unordered_map<std::string, State> _states;
  std::unordered_set<std::string> _snapshot;
  Callbacks _state_callbacks;
  async::Lifetime _load_work, _save_work;
};

}  // namespace web_proxy
