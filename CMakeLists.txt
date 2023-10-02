cmake_minimum_required(VERSION 3.16)

project(spoticode)

if (PI)
  add_subdirectory(spidev-lib)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND CMAKE_SYSTEM_PREFIX_PATH /opt/homebrew)

find_package(CURL REQUIRED)
find_library(JQ NAMES libjq.dylib REQUIRED)
find_path(JQ_INCLUDE_DIR jq.h REQUIRED)
find_path(ASIO_INCLUDE_DIR asio.h REQUIRED)

add_subdirectory(src)

set(SOURCES
  font/font.cpp
  font/font.h
  storage/string_set.cpp
  storage/string_set.h
  util/color.h
  util/signal_handler.cpp
  util/signal_handler.h
  util/text_prompt.cpp
  util/text_prompt.h
  util/url.cpp
  util/url.h
  apa102.cpp
  apa102.h
  main.cpp
  spotify_client.cpp
  spotify_client.h
  spotiled.cpp
  spotiled.h
)

list(TRANSFORM SOURCES PREPEND "src/")

add_executable(spoticode ${SOURCES})

target_include_directories(spoticode PRIVATE
  ${CURL_INCLUDE_DIR}
  ${JQ_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(spoticode
  # 1st party
  async
  http
  http_server
  present
  settings
  # 3rd party
  pthread
  ${CURL_LIBRARIES}
  ${JQ}
)

if (PI)
  target_link_libraries(spoticode spidev-lib++)
endif()

add_executable(string_set_test
  src/storage/string_set.cpp
  src/storage/string_set.h
  src/tests/storage/string_set_test.cpp
)
target_include_directories(string_set_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
