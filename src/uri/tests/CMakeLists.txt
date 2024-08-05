
set(SOURCES
  uri_test.cpp
)

add_executable(uri_test ${SOURCES})

target_include_directories(uri_test PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(uri_test
  uri
)
