
add_executable(tictactoe
  tictactoe.c)

target_link_libraries(tictactoe
  PRIVATE
    casino)

add_executable(break
  breakthrough.c)

target_link_libraries(break
  PRIVATE
    casino)

add_executable(amazons
  amazons.c)

target_link_libraries(amazons
  PRIVATE
    casino)
