
set(SOURCES
  string_set_test.cpp
)

add_executable(storage_test ${SOURCES})

target_include_directories(storage_test PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(storage_test
  storage
)
