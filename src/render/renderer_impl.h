#pragma once

#include <async/scheduler.h>
#include <render/renderer.h>

namespace render {

struct BufferedLED : LED {
  virtual void clear() = 0;
  virtual void show() = 0;
};

std::unique_ptr<Renderer> createRenderer(async::Scheduler &main_scheduler,
                                         std::unique_ptr<BufferedLED>);

}  // namespace render
