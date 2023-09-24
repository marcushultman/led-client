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
  std::signal(SIGINT, [](int sig) { s_handler->schedule(sig); });
}

SignalHandler::~SignalHandler() {
  std::signal(SIGINT, nullptr);
  s_handler = nullptr;
}

void SignalHandler::schedule(int sig) {
  _lifetime = _scheduler.schedule([this, sig] { _callback(sig); });
}

}  // namespace csignal
