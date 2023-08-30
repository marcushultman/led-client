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

struct Response {
  int status = 500;
  std::string body;
};

struct RequestInit {
  using OnResponse = std::function<void(Response)>;
  Method method = Method::GET;
  std::string url;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
  async::Scheduler &post_to;
  OnResponse callback;
};

struct Request {
  virtual ~Request() = default;
};

struct Http {
  virtual ~Http() = default;
  virtual std::unique_ptr<Request> request(RequestInit) = 0;

  static std::unique_ptr<Http> create();
};

}  // namespace http
