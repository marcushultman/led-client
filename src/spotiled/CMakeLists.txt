
set(SOURCES
  spotiled.h
  spotiled.cpp
)

add_library(spotiled ${SOURCES})

target_include_directories(spotiled PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(spotiled
  apa102
  render
  color
  settings
)

add_executable(spoticode main.cpp)

target_include_directories(spoticode PRIVATE
  ${CURL_INCLUDE_DIR}
  ${JQ_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(spoticode
  # 1st party
  async
  color
  csignal
  http
  http_server
  ikea
  present
  program_options
  settings
  spotiled
  uri
  web_proxy
  # 3rd party
  pthread
  ${CURL_LIBRARIES}
  ${JQ}
)
