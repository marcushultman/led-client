#pragma once

#include "spotiled.h"

namespace font {

struct TextPage : Page {
  virtual void setText(const std::string &) = 0;

  static std::unique_ptr<TextPage> create();
};

int run();

}  // namespace font
