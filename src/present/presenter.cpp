#include "presenter.h"

#include <map>
#include <queue>

#include "util/spotiled/spotiled.h"

namespace present {

struct PresenterQueueImpl final : PresenterQueue {
  explicit PresenterQueueImpl(SpotiLED &led) : _led{led} {}

  void add(Presentable &presentable, const Options &options = {}) final {
    if (_current.presentable && _current.prio < options.prio) {
      auto presentable = std::exchange(_current.presentable, nullptr);
      presentable->stop();
      _queue[_current.prio].push_front(presentable);
    }
    _queue[options.prio].push_back(&presentable);
    if (!_current.presentable) {
      presentNext();
    }
  }
  void erase(Presentable &presentable) final {
    for (auto &[_, queue] : _queue) {
      std::erase(queue, &presentable);
    }
    if (_current.presentable == &presentable) {
      presentNext();
    }
  }
  void notify() final { _led.notify(); }

  void clear() final {
    _current.presentable = nullptr;
    _queue.clear();
  }

 private:
  void presentNext() {
    for (auto &[prio, queue] : _queue) {
      if (queue.empty()) {
        continue;
      }
      _current.prio = prio;
      _current.presentable = queue.front();
      queue.pop_front();
      _current.presentable->start(_led, [this, presentable = _current.presentable] {
        if (_current.presentable == presentable) {
          presentNext();
        }
      });
      return;
    }
    _current.presentable = nullptr;
    _led.notify();
  }

  struct Current {
    Prio prio;
    Presentable *presentable = nullptr;
  };
  SpotiLED &_led;
  std::map<Prio, std::deque<Presentable *>, std::greater<Prio>> _queue;
  Current _current;
};

std::unique_ptr<PresenterQueue> makePresenterQueue(SpotiLED &led) {
  return std::make_unique<PresenterQueueImpl>(led);
}

}  // namespace present
