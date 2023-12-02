
set(SOURCES
  gfx.h
  dummy.cpp
)

add_library(gfx ${SOURCES})

target_include_directories(gfx PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(gfx
)
