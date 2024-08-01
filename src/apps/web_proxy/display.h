#pragma once

#include <chrono>
#include <string>

#include "present/presenter.h"
#include "util/color/color.h"

namespace web_proxy {

struct Display final : present::Presentable {
  Color logo = color::kWhite;
  std::string bytes;
  present::Prio prio = present::Prio::kApp;

  size_t width = {};
  size_t height = {};
  int xscroll = {};

  void onRenderPass(spotiled::LED &, std::chrono::milliseconds) final;
};

}  // namespace web_proxy
