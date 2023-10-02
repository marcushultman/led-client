
set(SOURCES
  scheduler.h
  scheduler.cpp
)

add_library(async ${SOURCES})

target_include_directories(async PRIVATE
  ${ROOT_DIR}/src
)
