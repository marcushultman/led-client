#pragma once

#include <async/scheduler.h>
#include <spotiled/renderer.h>

#include "brightness_provider.h"

namespace spotiled {

std::unique_ptr<Renderer> create(async::Scheduler &main_scheduler, BrightnessProvider &);

}  // namespace spotiled
