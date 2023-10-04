
set(SOURCES
  url.h
  url.cpp
)

add_library(url ${SOURCES})

target_include_directories(url PRIVATE
  ${ROOT_DIR}/src
)
