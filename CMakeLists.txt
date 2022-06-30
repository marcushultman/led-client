cmake_minimum_required(VERSION 3.16)

project(spoticode)

add_subdirectory(spidev-lib)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)
find_library(JQ jq)

# boo
include_directories(spidev-lib/src/)

set(SOURCES
  apa102.cpp
  apa102.h
  main.cpp
  spotify_client.cpp
  spotify_client.h
)

list(TRANSFORM SOURCES PREPEND "src/")

add_executable(spoticode ${SOURCES})

include_directories(/usr/local/include ${CURL_INCLUDE_DIR})
target_link_libraries(spoticode pthread ${CURL_LIBRARIES} ${JQ})

message(STATUS "PI: ${PI}")

if (PI)
  target_link_libraries(spoticode
    spidev-lib++
  )
endif()
