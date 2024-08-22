
set(SOURCES
  scheduler.h
  scheduler.cpp
)

add_library(async ${SOURCES})

target_include_directories(async PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

add_subdirectory(tests)
