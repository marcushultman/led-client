
#include "service/service_registry.h"

#include <future>
#include <iostream>

#include "async/scheduler.h"

int main() {
  struct FooService {
    FooService() : value{42} {}
    int foo() { return value; }

    int value;
  };

  struct BarService {
    BarService(FooService &foo) : foo{foo} {}
    void bar() { std::cout << foo.foo(); }
    FooService &foo;
  };

  int num_foo = 0;
  int num_bar = 0;

  auto thread = async::Thread::create();
  auto api = service::makeServices({
      {"foo",
       [&](auto &) {
         ++num_foo;
         return std::make_unique<FooService>();
       }},
      {"bar",
       [&](service::Ctx &ctx) {
         ++num_bar;
         return std::make_unique<BarService>(ctx.inject<FooService>("foo"));
       }},
  });

  auto bar = api->startService<BarService>("bar");
  auto foo = api->startService<FooService>("foo");
  bar.reset();
  bar = api->startService<BarService>("bar");

  assert(num_foo == 1);
  assert(num_bar == 2);
}