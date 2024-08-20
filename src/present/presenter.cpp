#include "presenter.h"

#include <deque>
#include <iostream>
#include <map>
#include <utility>

namespace present {

using namespace std::chrono_literals;

struct Entry {
  Presentable *presentable = nullptr;
  Options options;
  bool operator==(const Presentable *rhs) const { return presentable == rhs; }
};

struct RenderHandle {
  RenderHandle(render::Renderer &renderer, Entry entry) : entry{std::move(entry)} {
    std::cout << "presentable onStart " << entry.presentable << std::endl;
    entry.presentable->onStart();
    renderer.add([this, alive = std::weak_ptr<void>(_alive)](
                     auto &led, auto elapsed) -> std::chrono::milliseconds {
      if (!alive.expired()) {
        this->entry.presentable->onRenderPass(led, elapsed);
        return this->entry.options.render_period;
      }
      return {};
    });
  }
  ~RenderHandle() {
    std::cout << "presentable onStop " << entry.presentable << std::endl;
    entry.presentable->onStop();
  }

  Entry entry;

 private:
  std::shared_ptr<void> _alive = std::make_shared<bool>(1);
};

struct PresenterImpl final : Presenter {
  PresenterImpl(std::unique_ptr<render::Renderer> renderer) : _renderer{std::move(renderer)} {}

  void add(Presentable &presentable, const Options &options = {}) final {
    if (_current && _current->entry.options.prio < options.prio) {
      auto current = std::exchange(_current, {});
      _queue[current->entry.options.prio].push_front(current->entry);
    }
    std::cout << "adding " << &presentable << std::endl;
    _queue[options.prio].push_back({&presentable, options});
    if (!_current) {
      presentNext();
    }
  }

  void erase(Presentable &presentable) final {
    std::cout << "erase " << &presentable << std::endl;

    for (auto &[_, queue] : _queue) {
      std::erase(queue, &presentable);
    }
    if (_current && _current->entry == &presentable) {
      _current.reset();
      presentNext();
    }
  }

  void notify() final { _renderer->notify(); }

 private:
  void presentNext() {
    for (auto &[prio, queue] : _queue) {
      if (queue.empty()) {
        continue;
      }
      _current = std::make_unique<RenderHandle>(*_renderer, std::move(queue.front()));
      queue.pop_front();
      return;
    }
    _renderer->notify();
  }

  std::unique_ptr<render::Renderer> _renderer;
  std::map<Prio, std::deque<Entry>, std::greater<Prio>> _queue;
  std::unique_ptr<RenderHandle> _current;
};

std::unique_ptr<Presenter> makePresenter(std::unique_ptr<render::Renderer> renderer) {
  return std::make_unique<PresenterImpl>(std::move(renderer));
}

}  // namespace present
