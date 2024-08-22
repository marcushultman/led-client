
set(SOURCES
  string_set_test.cpp
)

add_executable(storage_test ${SOURCES})

target_include_directories(storage_test PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(storage_test
  storage
)
