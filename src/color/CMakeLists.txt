
set(SOURCES
  color.h
  dummy.cpp
)

add_library(color ${SOURCES})

target_include_directories(color PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
