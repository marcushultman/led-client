
set(SOURCES
  string_set.h
  string_set.cpp
)

add_library(storage ${SOURCES})

target_include_directories(storage PRIVATE
  ${ROOT_DIR}/src
)

add_subdirectory(tests)
