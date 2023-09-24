#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace service {

struct Ctx {
  virtual ~Ctx() = default;
  virtual void *injectImpl(const std::string &id) = 0;

  template <typename T>
  T &inject(const std::string &id) {
    return *reinterpret_cast<T *>(injectImpl(id));
  }
};

using ServiceFactory = std::function<std::shared_ptr<void>(Ctx &)>;

class Services final {
 public:
  Services(std::unordered_map<std::string, ServiceFactory>);

  template <typename T>
  std::shared_ptr<T> startService(std::string id) {
    return std::reinterpret_pointer_cast<T>(startServiceInner(id));
  }

 private:
  struct KeepAlive;

  std::shared_ptr<void> startServiceInner(std::string id);
  std::shared_ptr<void> getOrCreateService(KeepAlive &keep_alive, std::string id);

  std::unordered_map<std::string, ServiceFactory> _factories;
  std::unordered_map<std::string, std::weak_ptr<void>> _services;
};

std::unique_ptr<Services> makeServices(std::unordered_map<std::string, ServiceFactory>);

}  // namespace service
