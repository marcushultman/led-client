cmake_minimum_required(VERSION 3.16)

project(spoticode)

if (PI)
  add_subdirectory(spidev-lib)
  # add_compile_definitions("-Wno-psabi")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_PREFIX_PATH /opt/homebrew)

find_package(CURL REQUIRED)
find_library(JQ NAMES libjq.dylib libjq.so REQUIRED)
find_path(JQ_INCLUDE_DIR jq.h REQUIRED)
find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)

add_subdirectory(src)
