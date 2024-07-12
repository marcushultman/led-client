#include "presenter.h"

#include <deque>
#include <iostream>
#include <map>

namespace present {

using namespace std::chrono_literals;

struct PresenterQueueImpl final : PresenterQueue {
  PresenterQueueImpl(async::Scheduler &main_scheduler, spotiled::BrightnessProvider &brightness)
      : _renderer{spotiled::Renderer::create(main_scheduler, brightness)} {
    _renderer->add([this](auto &led, auto elapsed) -> std::chrono::milliseconds {
      if (!_current.presentable) {
        return 1h;
      }
      if (!_current.start) {
        _current.start = elapsed;
      }
      _current.presentable->onRenderPass(led, elapsed - *_current.start);
      return _current.options.render_period;
    });
  }

  void add(Presentable &presentable, const Options &options = {}) final {
    std::cout << "adding " << &presentable << std::endl;

    if (_current.presentable && _current.options.prio < options.prio) {
      auto current = std::exchange(_current, {});
      std::cout << "stopping " << current.presentable << std::endl;
      current.presentable->onStop();
      current.start = {};
      _queue[current.options.prio].push_front(current);
    }
    _queue[options.prio].push_back({&presentable, options});
    if (!_current.presentable) {
      presentNext();
    }
  }
  void erase(Presentable &presentable) final {
    for (auto &[_, queue] : _queue) {
      std::erase(queue, &presentable);
    }
    if (_current.presentable == &presentable) {
      _current = {};
      std::cout << "erase (current) " << &presentable << std::endl;
      presentable.onStop();
      presentNext();
    } else {
      std::cout << "erase " << &presentable << std::endl;
    }
  }

  void clear() final {
    std::cout << "clear" << std::endl;
    _current = {};
    _queue.clear();
  }

  void notify() final { _renderer->notify(); }

 private:
  void presentNext() {
    for (auto &[prio, queue] : _queue) {
      if (queue.empty()) {
        continue;
      }
      _current = std::move(queue.front());
      queue.pop_front();
      std::cout << "presenting " << _current.presentable << std::endl;
      _current.presentable->onStart();
      _renderer->notify();
      return;
    }
    _current.presentable = nullptr;
  }

  struct Entry {
    Presentable *presentable = nullptr;
    Options options;
    std::optional<std::chrono::milliseconds> start;
    bool operator==(const Presentable *rhs) const { return presentable == rhs; }
  };

  std::unique_ptr<spotiled::Renderer> _renderer;
  std::map<Prio, std::deque<Entry>, std::greater<Prio>> _queue;
  Entry _current;
};

std::unique_ptr<PresenterQueue> makePresenterQueue(async::Scheduler &main_scheduler,
                                                   spotiled::BrightnessProvider &brightness) {
  return std::make_unique<PresenterQueueImpl>(main_scheduler, brightness);
}

}  // namespace present
