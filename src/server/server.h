#pragma once

#include <functional>
#include <memory>

#include "async/scheduler.h"

struct Server {
  virtual ~Server() = default;
  virtual int port() const = 0;
};

using OnRequest = std::function<void()>;

std::unique_ptr<Server> makeServer(async::Scheduler &, OnRequest);
