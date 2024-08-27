
set(SOURCES
  signal_catcher.h
  signal_catcher.cpp
  signal_handler.h
  signal_handler.cpp
)

add_library(csignal ${SOURCES})

target_include_directories(csignal PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(csignal
  async
)
