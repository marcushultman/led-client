
set(SOURCES
  settings.h
  updater.h
  updater.cpp
)

add_library(settings ${SOURCES})

target_include_directories(settings PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(settings
  encoding
  jq
)
