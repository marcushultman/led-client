#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include "async/scheduler.h"
#include "http/http.h"

namespace http {

struct Server {
  virtual ~Server() = default;
  virtual int port() const = 0;
};

using OnRequest = std::function<void(Request)>;

std::unique_ptr<Server> makeServer(async::Scheduler &, OnRequest);

}  // namespace http
