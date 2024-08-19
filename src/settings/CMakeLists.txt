
set(SOURCES
  settings.h
  updater.h
  updater.cpp
)

add_library(settings ${SOURCES})

target_include_directories(settings PRIVATE
  ${ROOT_DIR}/src
  ${JQ_INCLUDE_DIR}
)
target_link_libraries(settings
  encoding
  ${JQ}
)
