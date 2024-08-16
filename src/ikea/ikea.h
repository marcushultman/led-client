#pragma once

#include <async/scheduler.h>
#include <spotiled/brightness_provider.h>
#include <spotiled/renderer.h>

namespace ikea {

std::unique_ptr<spotiled::Renderer> create(async::Scheduler &main_scheduler,
                                           spotiled::BrightnessProvider &);

}  // namespace ikea
