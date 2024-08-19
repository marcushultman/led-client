#pragma once

#include <async/scheduler.h>
#include <settings/settings.h>
#include <spotiled/renderer.h>

namespace spotiled {

std::unique_ptr<Renderer> create(async::Scheduler &main_scheduler, const settings::Settings &);

}  // namespace spotiled
