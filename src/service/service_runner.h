#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "http/http.h"
#include "service_registry.h"

namespace service {

struct API {
  virtual ~API() = default;
  virtual std::chrono::milliseconds timeout(const http::Request &);
  virtual http::Response handleRequest(http::Request);
};

struct Config {
  bool ignore_requests = false;
  std::chrono::milliseconds timeout;
};
using ConfigMap = std::unordered_map<std::string, Config>;

struct ServiceRunner {
  virtual ~ServiceRunner() = default;
  virtual std::optional<http::Response> handleRequest(http::Request) = 0;
};

std::unique_ptr<ServiceRunner> makeServiceRunner(async::Scheduler &scheduler,
                                                 Services &services,
                                                 ConfigMap = {});

}  // namespace service
