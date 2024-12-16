
set(SOURCES
  button_reader.h
  button_reader.cpp
  ikea.h
  ikea.cpp
)

add_library(ikea_led ${SOURCES})

target_include_directories(ikea_led PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

if (APPLE)
  target_compile_definitions(ikea_led PUBLIC "WITH_SIMULATOR=1")
endif()

target_link_libraries(ikea_led
  async
  color
  render
)

if (PI)
  target_link_libraries(ikea_led
    pigpiod_if2
    spidev-lib++
  )
endif()

add_executable(ikea main.cpp)

target_include_directories(ikea PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(ikea
  # 1st party
  async
  color
  csignal
  http
  http_server
  ikea_led
  program_options
  uri
  web_proxy
  # 3rd party
  pthread
  curl
  jq
)
