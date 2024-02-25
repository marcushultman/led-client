
set(SOURCES
  base64.h
  dummy.cpp
)

add_library(encoding ${SOURCES})

target_include_directories(encoding PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(encoding
)
