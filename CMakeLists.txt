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
find_library(JQ REQUIRED)
find_path(JQ_INCLUDE_DIR jq.h REQUIRED)

set(SOURCES
  apa102.cpp
  apa102.h
  main.cpp
  spotify_client.cpp
  spotify_client.h
)

list(TRANSFORM SOURCES PREPEND "src/")

add_executable(spoticode ${SOURCES})

target_include_directories(spoticode PRIVATE
  ${CURL_INCLUDE_DIR}
  ${JQ_INCLUDE_DIR}
)

target_link_libraries(spoticode
  pthread
  ${CURL_LIBRARIES}
  ${JQ}
)

if (PI)
  target_link_libraries(spoticode spidev-lib++)
endif()
