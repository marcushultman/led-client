#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include "async/scheduler.h"

struct Server {
  virtual ~Server() = default;
  virtual int port() const = 0;
};

struct ServerRequest {
  std::string_view method;
  std::string_view path;
  std::unordered_map<std::string, std::string_view> headers;
  std::string_view body;
};

using OnRequest = std::function<void(ServerRequest)>;

std::unique_ptr<Server> makeServer(async::Scheduler &, OnRequest);
