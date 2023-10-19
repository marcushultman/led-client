#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace async {

using Lifetime = std::shared_ptr<void>;
using Fn = std::function<void()>;

struct Scheduler {
  struct Options {
    std::chrono::microseconds delay;
    std::chrono::microseconds period;
  };

  virtual ~Scheduler() = default;
  [[nodiscard]] virtual Lifetime schedule(Fn &&fn, const Options &options = {}) = 0;
};

struct Thread {
  virtual ~Thread() = default;
  virtual Scheduler &scheduler() = 0;

  static std::unique_ptr<Thread> create(std::string_view name = "");
};

}  // namespace async
