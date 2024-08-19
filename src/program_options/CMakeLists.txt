
set(SOURCES
  program_options.h
  program_options.cpp
)

add_library(program_options ${SOURCES})

target_include_directories(program_options PRIVATE
  ${ROOT_DIR}/src
)
target_link_libraries(program_options
)
