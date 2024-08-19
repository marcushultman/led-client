#pragma once

#include <string_view>

#include "settings.h"

namespace settings {

struct Updater {
  explicit Updater(Settings &);

  void update(std::string_view data);

 private:
  Settings &_settings;
};

}  // namespace settings
