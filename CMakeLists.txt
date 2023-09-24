cmake_minimum_required(VERSION 3.16)

project(spoticode)

if (PI)
  add_subdirectory(spidev-lib)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH /opt/homebrew)

find_package(CURL REQUIRED)
find_library(JQ NAMES libjq.dylib REQUIRED)
find_path(JQ_INCLUDE_DIR jq.h REQUIRED)

set(SOURCES
  async/scheduler.cpp
  async/scheduler.h
  font/font.cpp
  font/font.h
  http/http.cpp
  http/http.h
  server/server.cpp
  server/server.h
  storage/string_set.cpp
  storage/string_set.h
  util/color.h
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

add_executable(service_test
  src/async/scheduler.cpp
  src/async/scheduler.h
  src/service/service_registry.cpp
  src/service/service_registry.h
  src/service/test/service_registry_test.cpp
)
target_include_directories(service_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
