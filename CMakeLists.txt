
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(
  cosas
  VERSION 0.0
  DESCRIPTION "just trying to get cmake working"
  LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(workshop-computer
                     GIT_REPOSITORY https://github.com/TomWhitwell/Workshop_Computer.git
                     GIT_TAG        main)
FetchContent_MakeAvailable(workshop-computer)

add_subdirectory(src)

set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(cosas hello-world.cpp)
target_compile_features(cosas PRIVATE cxx_std_20)
target_link_libraries(cosas PRIVATE cosas_lib)


