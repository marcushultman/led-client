#include "service_registry.h"

#include <unordered_set>

namespace service {

// KeepAlive

struct Services::KeepAlive final : Ctx {
  KeepAlive(Services &services) : _services{services} {}

  void *injectImpl(const std::string &id) { return _services.getOrCreateService(*this, id).get(); }
  void insert(std::shared_ptr<void> instance) { _instances.insert(instance); }

 private:
  Services &_services;
  std::unordered_set<std::shared_ptr<void>> _instances;
};

// Services

Services::Services(std::unordered_map<std::string, ServiceFactory> factories) {
  _factories = std::move(factories);
}

std::shared_ptr<void> Services::startServiceInner(std::string id) {
  auto keep_alive = std::make_shared<KeepAlive>(*this);
  if (auto service = getOrCreateService(*keep_alive, id)) {
    return std::shared_ptr<void>(service.get(),
                                 [keep_alive](auto *) mutable { keep_alive.reset(); });
  }
  return nullptr;
}

std::shared_ptr<void> Services::getOrCreateService(KeepAlive &keep_alive, std::string id) {
  if (auto service = _services[id].lock()) {
    keep_alive.insert(service);
    return service;
  }
  auto &factory = _factories[id];
  if (!factory) {
    return nullptr;
  }
  auto service = factory(keep_alive);
  keep_alive.insert(service);
  _services[id] = service;
  return service;
}

std::unique_ptr<Services> makeServices(std::unordered_map<std::string, ServiceFactory> factories) {
  return std::make_unique<Services>(std::move(factories));
}

}  // namespace service
