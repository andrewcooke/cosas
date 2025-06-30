
cmake_minimum_required(VERSION 3.13)

if (BUILD STREQUAL "pico")

   #set(PICO_BOARD mtm_computer_16mb CACHE STRING "Board type")
   set(PICO_BOARD pico CACHE STRING "Board type")
   set(PICO_SDK_FETCH_FROM_GIT on)
   include(pico_sdk_import.cmake)

   pico_sdk_init()

endif()

project(cosas VERSION 0.0 DESCRIPTION "just trying to get cmake working" LANGUAGES CXX C)   # C for sdk - https://stackoverflow.com/a/74591212

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)

if (BUILD STREQUAL "pico")

    add_executable(holawea hello-world.cpp)
    target_compile_features(holawea PRIVATE cxx_std_20)
    target_link_libraries(holawea PRIVATE pico_stdlib cosas_lib)
    pico_add_extra_outputs(holawea)   # generate uf2
    pico_enable_stdio_usb(holawea 1)  # allow serial over usb

else()

    include(FetchContent)
    FetchContent_Declare(doctest GIT_REPOSITORY "https://github.com/onqtam/doctest" GIT_TAG "master")
    FetchContent_MakeAvailable(doctest)	

    # tests, run_tests
    add_subdirectory(test)

    # dump
    add_subdirectory(dump)

endif()
