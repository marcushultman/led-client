
set(SOURCES
  ui_service.h
  ui_service.cpp
)

add_library(ui ${SOURCES})

target_include_directories(ui PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(ui
  async
  font
  http
  present
  settings
  spotiled
)
