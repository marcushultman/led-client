
set(SOURCES
  authenticator_presenter.cpp
  authenticator_presenter.h
  now_playing_presenter.cpp
  now_playing_presenter.h
  now_playing_service.cpp
  now_playing_service.h
  service.cpp
  service.h
)

add_library(spotify ${SOURCES})

target_include_directories(spotify PRIVATE
  ${JQ_INCLUDE_DIR}
  ${ROOT_DIR}/src
)
target_link_libraries(spotify
  async
  font
  http
  jq_util
  present
  settings
  storage
  url
  ${JQ}
)

add_subdirectory(tests)
