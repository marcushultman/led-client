
set(SOURCES
  led.h
  led.cpp
)

add_library(led ${SOURCES})

target_include_directories(led PRIVATE
  ${ROOT_DIR}/src
)
