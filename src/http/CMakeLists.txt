add_subdirectory(server)

set(SOURCES
  http.h
  http.cpp
)

add_library(http ${SOURCES})

target_include_directories(http PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(http
  async
  ${CURL_LIBRARIES}
)
