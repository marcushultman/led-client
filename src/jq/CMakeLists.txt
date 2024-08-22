
set(SOURCES
  jq_util.h
  jq_util.cpp
)

add_library(jq_util ${SOURCES})

target_include_directories(jq_util PRIVATE
  ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(jq_util
  jq
)
