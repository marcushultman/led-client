#pragma once

#include <memory>
#include <string>

#include "util/page/page.h"

namespace font {

struct TextPage : page::Page {
  virtual void setText(const std::string &) = 0;

  static std::unique_ptr<TextPage> create();
};

int run();

}  // namespace font
