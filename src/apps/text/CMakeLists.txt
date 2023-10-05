
set(SOURCES
  text_service.h
  text_service.cpp
)

add_library(text ${SOURCES})

target_include_directories(text PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(text
  font
  http
  present
  settings
  spotiled
)
