#pragma once

#include <async/scheduler.h>
#include <settings/settings.h>
#include <spotiled/renderer.h>

namespace ikea {

std::unique_ptr<spotiled::Renderer> create(async::Scheduler &main_scheduler,
                                           const settings::Settings &);

}  // namespace ikea
