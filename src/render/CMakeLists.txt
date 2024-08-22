
set(SOURCES
  renderer.h
  renderer_impl.h
  renderer_impl.cpp
  time_of_day_brightness.h
)

add_library(render ${SOURCES})

target_include_directories(render PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(render
  async
  color
  led
  settings
)
