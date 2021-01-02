
include(FetchContent)

FetchContent_Declare(
  ropemaker
  GIT_REPOSITORY https://github.com/AlexKent3141/ropemaker.git
  GIT_TAG v0.1)

FetchContent_MakeAvailable(ropemaker)

include_directories(${ropemaker_SOURCE_DIR}/include)

add_library(casino
  SHARED
    actions.c
    casino.c
    mcts.c
    memory.c
    node.c
    playout.c
    random.c
    selection.c)

set_target_properties(casino
  PROPERTIES
    C_VISIBILITY_PRESET hidden)

if (MSVC)
  # TODO
else()
  target_link_libraries(casino
    PRIVATE
      ropemaker
      m)
endif()
