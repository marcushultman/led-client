#pragma once

#include <memory>

struct Server {
  virtual ~Server() = default;
  virtual int port() const = 0;
};

std::unique_ptr<Server> makeServer();
