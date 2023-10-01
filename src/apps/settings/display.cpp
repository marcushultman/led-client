#include "display.h"

namespace settings {

DisplayService::DisplayService() = default;

http::Response DisplayService::operator()(http::Request req) {
  if (req.method != http::Method::POST || req.body.empty()) {
    return 400;
  }
  auto brightness = std::stoi(req.body);
  printf("brightness: %d\n", brightness);
  return 204;
}

}  // namespace settings
