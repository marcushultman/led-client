
set(SOURCES
  signal_handler.h
  signal_handler.cpp
)

add_library(csignal ${SOURCES})

target_include_directories(csignal PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(csignal
  async
)
