#pragma once

#include <unordered_set>

#include "async/scheduler.h"
#include "service_registry.h"

namespace service {

class ServiceRunner final {
 public:
  ServiceRunner(Services &services) : _services{services} {}

 private:
  Services &_services;
  std::unordered_set<async::Lifetime> _lifetimes;
};

}  // namespace service
