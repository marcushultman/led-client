
set(SOURCES
  ikea.h
  ikea.cpp
)

add_library(ikea ${SOURCES})

target_include_directories(ikea PRIVATE
  ${ROOT_DIR}/src
)

target_link_libraries(ikea
  async
  color
  spotiled
)

if (PI)
  target_link_libraries(ikea spidev-lib++)
endif()
