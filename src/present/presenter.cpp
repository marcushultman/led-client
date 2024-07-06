#include "presenter.h"

#include <deque>
#include <map>

namespace present {

struct PresenterQueueImpl final : PresenterQueue {
  void add(Presentable &presentable, const Options &options = {}) final {
    if (_current.presentable && _current.prio < options.prio) {
      auto presentable = std::exchange(_current.presentable, nullptr);
      presentable->onStop();
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
      _current.presentable->onStart();
      return;
    }
    _current.presentable = nullptr;
  }

  struct Current {
    Prio prio;
    Presentable *presentable = nullptr;
  };

  std::map<Prio, std::deque<Presentable *>, std::greater<Prio>> _queue;
  Current _current;
};

std::unique_ptr<PresenterQueue> makePresenterQueue() {
  return std::make_unique<PresenterQueueImpl>();
}

}  // namespace present
