
set(SOURCES
  spotiled.h
  spotiled.cpp
)

add_library(spotiled ${SOURCES})

target_include_directories(spotiled PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(spotiled
  apa102
)
