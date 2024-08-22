
set(SOURCES
  led.h
  led.cpp
)

add_library(led ${SOURCES})

target_include_directories(led PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
