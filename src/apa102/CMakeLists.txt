
set(SOURCES
  apa102.h
  apa102.cpp
)

add_library(apa102 ${SOURCES})

target_include_directories(apa102 PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(apa102
  led
)

if (PI)
  target_link_libraries(apa102 spidev-lib++)
endif()
