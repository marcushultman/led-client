
set(SOURCES
  http_test.cpp
)

add_executable(http_test ${SOURCES})

target_include_directories(http_test PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(http_test
  async
  http
)
