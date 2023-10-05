
set(SOURCES
  page.h
  page.cpp
)

add_library(page ${SOURCES})

target_include_directories(page PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(http
  gfx
)
