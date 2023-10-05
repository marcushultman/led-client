
set(SOURCES
  spotify_client_test.cpp
)

add_executable(spotify_test ${SOURCES})

target_include_directories(spotify_test PRIVATE
  ${JQ_INCLUDE_DIR}
  ${ROOT_DIR}/src
)
target_link_libraries(spotify_test
  apa102
  async
  font
  http
  present
  settings
  spotify
  url
)
