cmake_minimum_required (VERSION 3.16.3)
project(casino)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include_directories(
  "./include")

add_subdirectory(
  "./src")

add_subdirectory(
  "./games")

if (MSVC)
  # TODO
else()
  add_compile_options(
    -Wall -Wpedantic -Werror -ansi -O3)
endif()
