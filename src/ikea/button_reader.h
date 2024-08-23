#pragma once

#include <async/scheduler.h>

#include <functional>

namespace ikea {

struct ButtonReader {
  using OnPress = std::function<void()>;

  ButtonReader(async::Scheduler &main_scheduler, OnPress);
  ~ButtonReader();

 private:
  friend void s_onPress(int pi, unsigned user_gpio, unsigned level, uint32_t tick, void *reader);

  void onPress();

  async::Scheduler &_main_scheduler;
  OnPress _on_press;
  int _gpio = -1;
  int _callback = -1;
  async::Lifetime _work;
};

}  // namespace ikea
