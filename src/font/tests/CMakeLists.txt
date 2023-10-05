
set(SOURCES
  font_test.cpp
)

add_executable(font_test ${SOURCES})

target_include_directories(font_test PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(font_test
  font
)
