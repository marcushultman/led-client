cmake_minimum_required(VERSION 3.16)

project(spoticode)

if (PI)
  add_subdirectory(spidev-lib)
endif()

set(CMAKE_CXX_STANDARD 17)
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
