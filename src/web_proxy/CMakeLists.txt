
set(SOURCES
  display.h
  display.cpp
  state_thingy.h
  state_thingy.cpp
  web_proxy.h
  web_proxy.cpp
)

add_library(web_proxy ${SOURCES})

target_include_directories(web_proxy PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${JQ_INCLUDE_DIR}
)
target_link_libraries(web_proxy
  async
  encoding
  http
  ${JQ}
  present
  storage
  uri
)
