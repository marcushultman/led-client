cmake_minimum_required(VERSION 3.16)

project(spoticode)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
  apa102.h
  main.cpp
)

add_executable(spoticode ${SOURCES})

message(STATUS "PI: ${PI}")

if (PI)
  target_link_libraries(spoticode
    wiringPi
  )
endif()
