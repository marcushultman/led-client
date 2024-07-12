#pragma once

#include <util/spotiled/spotiled.h>

#include <chrono>
#include <memory>

namespace present {

struct Presentable {
  virtual ~Presentable() = default;
  virtual void onStart(){};
  virtual void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed){};
  virtual void onStop(){};
};

enum class Prio {
  kApp = 0,
  kNotification,
};
struct Options {
  Prio prio = Prio::kApp;
  std::chrono::milliseconds render_period = std::chrono::hours{1};
};

struct PresenterQueue {
  virtual ~PresenterQueue() = default;
  virtual void add(Presentable &, const Options & = {}) = 0;
  virtual void erase(Presentable &) = 0;
  virtual void clear() = 0;
  virtual void notify() = 0;
};

std::unique_ptr<PresenterQueue> makePresenterQueue(async::Scheduler &,
                                                   spotiled::BrightnessProvider &);

}  // namespace present
