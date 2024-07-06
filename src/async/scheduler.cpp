#include "scheduler.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>
#include <thread>

namespace async {

struct Entry {
  Fn fn;
  std::weak_ptr<void> sentinel;
  std::chrono::system_clock::time_point at;
  std::chrono::microseconds period;

  bool operator<(const Entry &rhs) const { return at < rhs.at; }
};

struct CVNotifier {
  explicit CVNotifier(std::shared_ptr<std::condition_variable> cv) : _cv{std::move(cv)} {}
  ~CVNotifier() { _cv->notify_all(); }

 private:
  std::shared_ptr<std::condition_variable> _cv;
};

class SchedulerImpl final : public Scheduler {
 public:
  Lifetime schedule(Fn &&fn, const Options &options = {}) final {
    auto sentinel = std::make_shared<CVNotifier>(_cv);
    {
      auto lock = std::unique_lock(_mutex);
      _queue.insert(Entry{
          .fn = std::move(fn),
          .sentinel = sentinel,
          .at = std::chrono::system_clock::now() + options.delay,
          .period = options.period,
      });
    }
    _cv->notify_all();
    return sentinel;
  }

  void run() {
    auto lock = std::unique_lock(_mutex);
    auto stop_waiting = [this] { return !_queue.empty() || _stop; };

    for (;;) {
      if (_queue.empty()) {
        _cv->wait(lock, stop_waiting);
      } else {
        _cv->wait_until(lock, _queue.begin()->at, stop_waiting);
      }

      if (_stop && _queue.empty()) {
        break;
      }

      auto queue = std::exchange(_queue, {});
      lock.unlock();

      auto now = std::chrono::system_clock::now();

      for (auto it = queue.begin(); it != queue.end();) {
        if (auto alive = it->sentinel.lock()) {
          if (it->at > now) {
            ++it;
            continue;
          }
          if (it->fn) {
            it->fn();
          }
          if (it->period.count()) {
            schedulerNext(*it);
          }
        }
        it = queue.erase(it);
      }

      lock.lock();
      _queue.insert(std::make_move_iterator(queue.begin()), std::make_move_iterator(queue.end()));
    }
  }

  void stop() {
    _stop = true;
    _cv->notify_all();
  }

 private:
  void schedulerNext(Entry entry) {
    entry.at += entry.period;
    auto lock = std::unique_lock(_mutex);
    _queue.insert(std::move(entry));
  }

  std::mutex _mutex;
  std::shared_ptr<std::condition_variable> _cv = std::make_shared<std::condition_variable>();
  std::atomic_bool _stop = false;
  std::set<Entry> _queue;
};

class ThreadImpl final : public Thread {
 public:
  explicit ThreadImpl(std::string_view name)
      : _thread{[this, name = std::string(name)] {
#if !__arm__
          pthread_setname_np(name.c_str());
#endif
          _scheduler->run();
        }} {
  }
  ~ThreadImpl() {
    _scheduler->stop();
    _thread.join();
  }

  Scheduler &scheduler() final { return *_scheduler; }

 private:
  std::unique_ptr<SchedulerImpl> _scheduler = std::make_unique<SchedulerImpl>();
  std::thread _thread;
};

std::unique_ptr<Thread> Thread::create(std::string_view name) {
  return std::make_unique<ThreadImpl>(name);
}

}  // namespace async
