
find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)

set(SOURCES
  server.h
  server.cpp
)

add_library(http_server ${SOURCES})

target_include_directories(http_server PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(http_server
  asio
  async
  http
)
