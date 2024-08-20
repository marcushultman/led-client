#pragma once

#include <async/scheduler.h>
#include <render/renderer.h>
#include <settings/settings.h>

namespace ikea {

std::unique_ptr<render::Renderer> create(async::Scheduler &main_scheduler,
                                         const settings::Settings &);

}  // namespace ikea
