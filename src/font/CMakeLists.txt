
set(SOURCES
  font.h
  font.cpp
)

add_library(font ${SOURCES})

target_include_directories(font PRIVATE
  ${ROOT_DIR}/src
)

add_subdirectory(tests)
