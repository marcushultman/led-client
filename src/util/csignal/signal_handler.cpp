#include "signal_handler.h"

#include <csignal>

namespace csignal {

SignalHandler *SignalHandler::s_handler = nullptr;

SignalHandler::SignalHandler(async::Scheduler &scheduler, Callback callback)
    : _scheduler{scheduler}, _callback{std::move(callback)} {
  if (s_handler) {
    std::terminate();
  }
  s_handler = this;
  auto catcher = [](int sig) { s_handler->schedule(sig); };
  std::signal(SIGINT, catcher);
  std::signal(SIGTERM, catcher);
}

SignalHandler::~SignalHandler() {
  std::signal(SIGTERM, nullptr);
  std::signal(SIGINT, nullptr);
  s_handler = nullptr;
}

void SignalHandler::schedule(int sig) {
  _lifetime = _scheduler.schedule([this, sig] {
    if (!_callback(sig)) {
      std::signal(sig, nullptr);
      std::raise(sig);
    }
  });
}

}  // namespace csignal
