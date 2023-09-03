#pragma once

#include <functional>
#include <memory>

#include "async/scheduler.h"

struct Server {
  virtual ~Server() = default;
  virtual int port() const = 0;
};

struct ServerRequest {
  std::string_view method;
  std::string_view path;
  std::string_view body;
  int action = -1;
};

using OnRequest = std::function<void(ServerRequest)>;

std::unique_ptr<Server> makeServer(async::Scheduler &, OnRequest);
