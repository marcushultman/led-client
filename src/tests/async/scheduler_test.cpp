
#include "async/scheduler.h"

#include <iostream>

int main(int argc, char *argv[]) {
  std::cout << "Thread::create()" << std::endl;
  auto thread = async::Thread::create();
  std::cout << "schedule(...)" << std::endl;
  (void)thread->scheduler().schedule([] { std::cout << "callback dropped" << std::endl; });
  std::cout << "scheduled" << std::endl;

  auto &scheduler = thread->scheduler();
  async::Lifetime l;

  auto i = 0;
  auto schedule = [&](auto f) {
    if (i > 3) {
      return;
    }
    std::cout << "callback " << ++i << std::endl;
    l = scheduler.schedule([f] { f(f); }, {.delay = std::chrono::seconds{1}});
  };

  auto foo = std::make_shared<std::string>("foo");

  l = scheduler.schedule([schedule] { schedule(schedule); }, {.delay = std::chrono::seconds{1}});

  std::cout << "reset" << std::endl;
  thread.reset();

  return 0;
}
