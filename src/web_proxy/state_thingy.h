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
#include "render/renderer.h"

namespace web_proxy {

struct State {
  std::string data;
  std::optional<Display> display;
  std::chrono::milliseconds retry_backoff = {};
  async::Lifetime work;
};

struct StateThingy final {
  using RequestUpdate =
      std::function<void(std::string id, State &, std::function<void()> on_update)>;

  StateThingy(async::Scheduler &main_scheduler,
              RequestUpdate request_update,
              std::unique_ptr<render::Renderer>);
  ~StateThingy();

  const std::unordered_map<std::string, State> &states();

  http::Lifetime handleRequest(http::Request &, http::RequestOptions &);
  void updateState(std::string id);

  void handleStateUpdate(const std::string &json);
  void onServiceResponse(http::Response, std::string id, State &);

 private:
  void loadStates();
  void saveStates();

  std::optional<http::Response> handleGetRequest(const http::Request &);
  http::Lifetime handlePostRequest(const http::Request &, http::RequestOptions);

  const std::string *findNextToDisplay() const;
  std::chrono::milliseconds onRender(render::LED &led, std::chrono::milliseconds elapsed);

  async::Scheduler &_main_scheduler;
  RequestUpdate _request_update;
  std::unique_ptr<render::Renderer> _renderer;
  std::unordered_map<std::string, State> _states;
  std::unordered_set<std::string> _snapshot;
  Display *_displaying = nullptr;
  async::Lifetime _load_work, _save_work;
};

}  // namespace web_proxy
