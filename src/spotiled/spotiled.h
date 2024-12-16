#pragma once

#include <async/scheduler.h>
#include <render/renderer.h>

namespace spotiled {

std::unique_ptr<render::Renderer> create(async::Scheduler &main_scheduler);

}  // namespace spotiled
