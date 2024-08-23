#include "button_reader.h"

#include <utility>

#if !WITH_SIMULATOR

#include <pigpiod_if2.h>

#endif

namespace ikea {

void s_onPress(int pi, unsigned user_gpio, unsigned level, uint32_t tick, void *reader) {
  static_cast<ButtonReader *>(reader)->onPress();
}

ButtonReader::ButtonReader(async::Scheduler &main_scheduler, OnPress on_press)
    : _main_scheduler{main_scheduler}, _on_press{std::move(on_press)} {
#if !WITH_SIMULATOR
  _gpio = pigpio_start(nullptr, nullptr);
  set_mode(_gpio, 21, PI_INPUT);
  set_pull_up_down(_gpio, 21, PI_PUD_UP);
  _callback = callback_ex(_gpio, 21, FALLING_EDGE, s_onPress, this);
#endif
}

ButtonReader::~ButtonReader() {
#if !WITH_SIMULATOR
  if (_callback >= 0) callback_cancel(_callback);
  if (_gpio >= 0) pigpio_stop(_gpio);
#endif
}

void ButtonReader::onPress() {
  _work = _main_scheduler.schedule([this] { _on_press(); });
}

}  // namespace ikea
