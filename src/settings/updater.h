#pragma once

#include <string_view>

#include "settings.h"

namespace settings {

void updateSettings(Settings &, std::string_view data);

}  // namespace settings
