
set(SOURCES
  brightness_provider.h
  spotiled.h
  spotiled.cpp
  time_of_day_brightness.h
)

add_library(spotiled ${SOURCES})

target_include_directories(spotiled PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(spotiled
  apa102
  color
)
