#include "presenter.h"

#include <deque>
#include <map>

#include "util/spotiled/spotiled.h"

namespace present {

struct PresenterQueueImpl final : PresenterQueue {
  explicit PresenterQueueImpl(spotiled::Renderer &renderer) : _renderer{renderer} {}

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
  void notify() final { _renderer.notify(); }

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
      _current.presentable->onStart(_renderer);
      return;
    }
    _current.presentable = nullptr;
    _renderer.notify();
  }

  struct Current {
    Prio prio;
    Presentable *presentable = nullptr;
  };
  spotiled::Renderer &_renderer;
  std::map<Prio, std::deque<Presentable *>, std::greater<Prio>> _queue;
  Current _current;
};

std::unique_ptr<PresenterQueue> makePresenterQueue(spotiled::Renderer &renderer) {
  return std::make_unique<PresenterQueueImpl>(renderer);
}

}  // namespace present
