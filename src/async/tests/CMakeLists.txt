
set(SOURCES
  scheduler_test.cpp
)

add_executable(async_test ${SOURCES})

target_include_directories(async_test PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(async_test
  async
)
