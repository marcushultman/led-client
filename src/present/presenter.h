#pragma once

#include <memory>

namespace present {

struct Presentable {
  virtual ~Presentable() = default;
  virtual void onStart() = 0;
  virtual void onStop() = 0;
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
  virtual void clear() = 0;
};

std::unique_ptr<PresenterQueue> makePresenterQueue();

}  // namespace present
