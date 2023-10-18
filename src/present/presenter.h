#pragma once

#include <functional>
#include <memory>

#include "util/spotiled/spotiled.h"

namespace present {

using Callback = std::function<void()>;

struct Presentable {
  virtual ~Presentable() = default;
  virtual void start(spotiled::LED &, Callback) = 0;
  virtual void stop() = 0;
};

enum class Prio {
  kApp = 0,
  kNotification,
};
struct Options {
  Prio prio = Prio::kApp;
};

struct PresenterQueue {
  virtual ~PresenterQueue() = default;
  virtual void add(Presentable &, const Options & = {}) = 0;
  virtual void erase(Presentable &) = 0;
  virtual void notify() = 0;
  virtual void clear() = 0;
};

std::unique_ptr<PresenterQueue> makePresenterQueue(spotiled::LED &);

}  // namespace present
