#include "signal_catcher.h"

namespace csignal {

SignalCatcher::SignalCatcher(async::Scheduler &scheduler,
                             std::vector<int> signals,
                             Callback callback)
    : _signals{std::move(signals)},
      _callback{std::move(callback)},
      _handler{scheduler, [this](auto sig) { return maybeTrigger(sig); }} {}

SignalCatcher::~SignalCatcher() = default;

bool SignalCatcher::maybeTrigger(int sig) {
  if (std::count(begin(_signals), end(_signals), sig)) {
    _callback(sig);
    return true;
  }
  return false;
}

}  // namespace csignal
