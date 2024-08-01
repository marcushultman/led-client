cmake_minimum_required(VERSION 3.16)

project(spoticode)

if (PI)
  add_subdirectory(spidev-lib)
endif()

include(FetchContent)

FetchContent_Declare(cpptrace
  GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  GIT_TAG        v0.4.1
)
FetchContent_MakeAvailable(cpptrace)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND CMAKE_SYSTEM_PREFIX_PATH /opt/homebrew)

find_package(CURL REQUIRED)
find_library(JQ NAMES libjq.dylib REQUIRED)
find_path(JQ_INCLUDE_DIR jq.h REQUIRED)
find_path(ASIO_INCLUDE_DIR asio.hpp REQUIRED)

add_subdirectory(src)

add_executable(spoticode src/main.cpp)

target_include_directories(spoticode PRIVATE
  ${CURL_INCLUDE_DIR}
  ${JQ_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(spoticode
  # 1st party
  async
  color
  csignal
  font
  http
  http_server
  page
  present
  settings
  spotify
  spotiled
  text
  url
  web_proxy
  # 3rd party
  cpptrace::cpptrace
  pthread
  ${CURL_LIBRARIES}
  ${JQ}
)
