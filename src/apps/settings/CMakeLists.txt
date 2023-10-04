
set(SOURCES
  brightness_provider.h
  display.h
  display.cpp
  time_of_day_brightness.h
)

add_library(settings ${SOURCES})

target_include_directories(settings PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(settings
  http
  present
  url
)
