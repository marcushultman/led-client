cmake_minimum_required(VERSION 3.20)

project(spoticode)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
  apa102.h
  main.cpp
)

add_executable(spoticode ${SOURCES})
