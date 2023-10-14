
set(SOURCES
  web_proxy.h
  web_proxy.cpp
)

add_library(web_proxy ${SOURCES})

target_include_directories(web_proxy PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(web_proxy
  async
  http
  url
)
