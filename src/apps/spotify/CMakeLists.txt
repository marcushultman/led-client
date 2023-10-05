
set(SOURCES
  service.h
  spotify_client.h
  spotify_client.cpp
)

add_library(spotify ${SOURCES})

target_include_directories(spotify PRIVATE
  ${JQ_INCLUDE_DIR}
  ${ROOT_DIR}/src
)
target_link_libraries(spotify
  async
  http
  present
  settings
  storage
  url
  ${JQ}
)

add_subdirectory(tests)
