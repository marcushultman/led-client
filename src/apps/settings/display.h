#pragma once

#include "http/http.h"

namespace settings {

struct DisplayService {
  DisplayService();

  http::Response operator()(http::Request);
};

}  // namespace settings
