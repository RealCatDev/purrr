cmake_minimum_required(VERSION 3.14)

project(purrr)

if (NOT TARGET glfw)
  add_subdirectory(deps/glfw)
endif()

file(GLOB_RECURSE SOURCES "src/**.c" "include/**.h")
add_library(purrr STATIC ${SOURCES})
target_link_libraries(purrr glfw)
target_include_directories(purrr PUBLIC include/)