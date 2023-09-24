#pragma once

#include <string>
#include <unordered_map>

#include "async/scheduler.h"

namespace http {

enum class Method {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
};

struct Request {
  Method method = Method::GET;
  std::string url;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
};

struct Response {
  int status = 500;
  std::string body;

  Response();
  Response(int status);
  Response(std::string body);
};

struct RequestOptions {
  using OnResponse = std::function<void(Response)>;
  async::Scheduler &post_to;
  OnResponse callback;
};

using Lifetime = std::shared_ptr<void>;

struct Http {
  virtual ~Http() = default;
  virtual Lifetime request(Request, RequestOptions) = 0;

  static std::unique_ptr<Http> create();
};

}  // namespace http
