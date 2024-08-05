#pragma once

#include <functional>

#include "async/scheduler.h"

namespace csignal {

class SignalHandler final {
 public:
  using Callback = std::function<bool(int)>;

  SignalHandler(async::Scheduler &scheduler, Callback callback);
  ~SignalHandler();

  void schedule(int sig);

 private:
  static SignalHandler *s_handler;
  async::Scheduler &_scheduler;
  Callback _callback;
  async::Lifetime _lifetime;
};

}  // namespace csignal
