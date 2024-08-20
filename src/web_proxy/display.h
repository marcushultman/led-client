#pragma once

#include <chrono>
#include <string>

#include "color/color.h"
#include "present/presenter.h"

namespace web_proxy {

struct Display final : present::Presentable {
  Color logo = color::kWhite;
  std::string bytes;
  present::Prio prio = present::Prio::kApp;

  size_t width = {};
  size_t height = {};
  int xscroll = {};
  double wave = {};

  void onRenderPass(render::LED &, std::chrono::milliseconds) final;
};

}  // namespace web_proxy
