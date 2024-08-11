
set(SOURCES
  display.h
  display.cpp
)

add_library(settings ${SOURCES})

target_include_directories(settings PRIVATE
  ${ROOT_DIR}/src
  ${JQ_INCLUDE_DIR}
)
target_link_libraries(settings
  http
  present
  spotiled
  uri
  ${JQ}
)
