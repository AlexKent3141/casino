
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

if (MSVC)
  # TODO
else()
  target_link_libraries(casino
    PRIVATE
      pthread
      m)
endif()
