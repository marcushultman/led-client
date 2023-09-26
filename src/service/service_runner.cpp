#include "service_runner.h"

#include <algorithm>
#include <map>

#include "async/scheduler.h"
#include "util/url.h"

namespace service {
namespace {

constexpr auto kDefaultConfig = Config{};

std::string parseServiceId(const http::Request &req) {
  auto url = url::Url(req.url);
  return url.path.size() ? std::string(url.path.back()) : "";
}

}  // namespace

std::chrono::milliseconds API::timeout(const http::Request &) { return {}; }

http::Response API::handleRequest(http::Request) { return 204; }

class ServiceRunnerImpl final : public ServiceRunner {
 public:
  ServiceRunnerImpl(async::Scheduler &scheduler, Services &services, ConfigMap config)
      : _scheduler{scheduler}, _services{services}, _config{std::move(config)} {}

  std::optional<http::Response> handleRequest(http::Request req) final {
    auto id = parseServiceId(req);
    auto service = _services.startService<API>(id);
    if (!service) {
      return {};
    }
    auto it = _config.find(id);
    auto &config = it != _config.end() ? it->second : kDefaultConfig;

    _lifetimes.erase(id);
    _lifetimes.emplace(id, service);

    http::Response res(204);
    auto timeout = config.timeout;

    if (!config.ignore_requests) {
      if (auto timeout_override = service->timeout(req); timeout_override.count()) {
        timeout = timeout_override;
      }
      res = service->handleRequest(std::move(req));
    }

    if (timeout.count()) {
      _lifetimes.emplace(
          id, _scheduler.schedule([this, id] { _lifetimes.erase(id); }, {.delay = timeout}));
    }

    return res;
  }

 private:
  async::Scheduler &_scheduler;
  Services &_services;
  ConfigMap _config;
  std::multimap<std::string, async::Lifetime> _lifetimes;
};

std::unique_ptr<ServiceRunner> makeServiceRunner(async::Scheduler &scheduler,
                                                 Services &services,
                                                 ConfigMap config) {
  return std::make_unique<ServiceRunnerImpl>(scheduler, services, std::move(config));
}

}  // namespace service
