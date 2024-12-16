#pragma once

#include <chrono>
#include <string>

#include "color/color.h"
#include "render/renderer.h"

namespace web_proxy {

struct Display final {
  enum class Prio {
    kApp = 0,
    kNotification,
  };

  Color logo = color::kWhite;
  std::string bytes;
  Prio prio = Prio::kApp;

  size_t width = {};
  size_t height = {};
  int xscroll = {};
  double wave = {};

  void onRenderPass(render::LED &, std::chrono::milliseconds);
};

}  // namespace web_proxy
