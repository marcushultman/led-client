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
  OPTIONS,
  UNKNOWN,
};

using Headers = std::unordered_map<std::string, std::string>;

struct Request {
  Method method = Method::GET;
  std::string url;
  Headers headers;
  std::string body;
};

struct Response {
  int status = 500;
  Headers headers;
  std::string body;

  Response();
  Response(int status);
  Response(std::string body);
};

struct RequestOptions {
  using OnResponse = std::function<void(Response)>;
  using OnBytes = std::function<void(int64_t offset, std::string_view, std::function<void()>)>;

  async::Scheduler &post_to;
  OnResponse on_response;
  OnBytes on_bytes;
};

using Lifetime = std::shared_ptr<void>;

struct Http {
  virtual ~Http() = default;
  virtual Lifetime request(Request, RequestOptions) = 0;

  static std::unique_ptr<Http> create();
};

}  // namespace http
