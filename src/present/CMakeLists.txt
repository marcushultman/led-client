
set(SOURCES
  presenter.h
  presenter.cpp
)

add_library(present ${SOURCES})

target_include_directories(present PRIVATE
  ${ROOT_DIR}/src
)
