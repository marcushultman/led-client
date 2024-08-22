
set(SOURCES
  uri.h
  uri.cpp
)

add_library(uri ${SOURCES})

target_include_directories(uri PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

add_subdirectory(tests)
