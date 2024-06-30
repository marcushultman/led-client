
set(SOURCES
  draw_service.h
  draw_service.cpp
)

add_library(draw ${SOURCES})

target_include_directories(draw PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(draw
  font
  http
  present
  settings
  spotiled
)
