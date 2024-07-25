#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <variant>

#include "async/scheduler.h"
#include "http/http.h"

namespace http {

struct Server {
  virtual ~Server() = default;
  virtual int port() const = 0;
};

using SyncHandler = std::function<Response(Request)>;
using AsyncHandler =
    std::function<Lifetime(Request, RequestOptions::OnResponse, RequestOptions::OnBytes)>;

using RequestHandler = std::variant<SyncHandler, AsyncHandler>;

std::unique_ptr<Server> makeServer(async::Scheduler &, RequestHandler, std::string base_url);

}  // namespace http
