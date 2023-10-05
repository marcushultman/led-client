
set(SOURCES
  apa102.h
  apa102.cpp
)

add_library(apa102 ${SOURCES})

target_include_directories(apa102 PRIVATE
  ${ROOT_DIR}/src
)
