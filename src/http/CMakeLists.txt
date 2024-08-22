add_subdirectory(server)

set(SOURCES
  http.cpp
  http.h
  util.cpp
  util.h
)

add_library(http ${SOURCES})

target_include_directories(http PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(http
  async
  curl
)

add_subdirectory(tests)
