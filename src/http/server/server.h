#pragma once

#include <async/scheduler.h>
#include <http/http.h>

#include <functional>
#include <memory>
#include <variant>

namespace http {

struct Server {
  virtual ~Server() = default;
  virtual int port() const = 0;
};

using SyncHandler = std::function<Response(Request)>;
using AsyncHandler = std::function<Lifetime(Request, RequestOptions)>;

using RequestHandler = std::variant<SyncHandler, AsyncHandler>;

std::unique_ptr<Server> makeServer(async::Scheduler &, RequestHandler);

}  // namespace http
