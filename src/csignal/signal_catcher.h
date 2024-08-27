#pragma once

#include <functional>
#include <vector>

#include "signal_handler.h"

namespace csignal {

class SignalCatcher final {
 public:
  using Callback = std::function<void(int)>;

  SignalCatcher(async::Scheduler &scheduler, std::vector<int> signals, Callback callback);
  ~SignalCatcher();

  bool maybeTrigger(int sig);

 private:
  std::vector<int> _signals;
  Callback _callback;
  SignalHandler _handler;
};

}  // namespace csignal
