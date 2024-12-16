
set(SOURCES
  renderer.h
  renderer_impl.h
  renderer_impl.cpp
)

add_library(render ${SOURCES})

target_include_directories(render PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(render
  async
  color
  led
)
