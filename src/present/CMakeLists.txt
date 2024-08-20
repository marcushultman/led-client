
set(SOURCES
  presenter.cpp
  presenter.h
)

add_library(present ${SOURCES})

target_include_directories(present PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(present
  async
  render
)
