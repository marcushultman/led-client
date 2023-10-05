
set(SOURCES
  presenter.cpp
  presenter.h
  rolling_presenter.cpp
  rolling_presenter.h
)

add_library(present ${SOURCES})

target_include_directories(present PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(present
  async
  color
  gfx
  page
  settings
  spotiled
)
