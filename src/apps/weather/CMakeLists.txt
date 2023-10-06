
set(SOURCES
  weather.h
  weather.cpp
)

add_library(weather ${SOURCES})

target_include_directories(weather PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(weather
  font
  apa102
)
