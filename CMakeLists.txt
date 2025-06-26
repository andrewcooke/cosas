
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD mtm_computer_16mb CACHE STRING "Board type")
set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(
  cosas
  VERSION 0.0
  DESCRIPTION "just trying to get cmake working"
  LANGUAGES CXX C)   # C for sdk - https://stackoverflow.com/a/74591212

include(FetchContent)
FetchContent_Declare(workshop-computer
                     GIT_REPOSITORY https://github.com/TomWhitwell/Workshop_Computer.git
                     GIT_TAG        main)
FetchContent_MakeAvailable(workshop-computer)

pico_sdk_init()

add_subdirectory(src)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(DOCTEST_CONFIG_DISABLE ON)
add_executable(cosas hello-world.cpp)
target_compile_features(cosas PRIVATE cxx_std_20)
target_compile_definitions(cosas PRIVATE DOCTEST_CONFIG_DISABLE)
target_link_libraries(cosas PRIVATE cosas_lib pico_stdlib)
pico_add_extra_outputs(cosas)
