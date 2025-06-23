
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(cosas)
project(
  my_project
  VERSION 0.0
  LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(workshop-computer
                     GIT_REPOSITORY https://github.com/TomWhitwell/Workshop_Computer.git
                     GIT_TAG        main)
FetchContent_MakeAvailable(workshop-computer)


