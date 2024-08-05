
set(SOURCES
  uri.h
  uri.cpp
)

add_library(uri ${SOURCES})

target_include_directories(uri PRIVATE
  ${ROOT_DIR}/src
)

add_subdirectory(tests)
