
cmake_minimum_required(VERSION 3.13)

#set(PICO_BOARD mtm_computer_16mb CACHE STRING "Board type")
set(PICO_BOARD pico CACHE STRING "Board type")
set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(
  cosas
  VERSION 0.0
  DESCRIPTION "just trying to get cmake working"
  LANGUAGES CXX C)   # C for sdk - https://stackoverflow.com/a/74591212

include(FetchContent)

#FetchContent_Declare(workshop-computer
#                     GIT_REPOSITORY https://github.com/TomWhitwell/Workshop_Computer.git
#                     GIT_TAG        main)
#FetchContent_MakeAvailable(workshop-computer)

pico_sdk_init()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# cosas_lib
add_subdirectory(src)

add_executable(holawea hello-world.cpp)
target_compile_features(holawea PRIVATE cxx_std_20)
target_compile_definitions(holawea PRIVATE DOCTEST_CONFIG_DISABLE)
target_link_libraries(holawea PRIVATE pico_stdlib cosas_lib)
pico_add_extra_outputs(holawea)   # generate uf2
pico_enable_stdio_usb(holawea 1)  # allow serial over usb

# tests, run_tests
add_subdirectory(test)
