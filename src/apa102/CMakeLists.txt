
set(SOURCES
  apa102.h
  apa102.cpp
)

add_library(apa102 ${SOURCES})

target_include_directories(apa102 PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(apa102
  led
)

if (APPLE)
  target_compile_definitions(apa102 PUBLIC "WITH_SIMULATOR=1")
endif()

if (PI)
  target_link_libraries(apa102 spidev-lib++)
endif()
