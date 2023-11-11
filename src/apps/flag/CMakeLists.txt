
set(SOURCES
  flag_service.h
  flag_service.cpp
)

add_library(flag ${SOURCES})

target_include_directories(flag PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(flag
  font
  http
  present
  settings
  spotiled
)
