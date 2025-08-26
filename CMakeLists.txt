
cmake_minimum_required(VERSION 3.13)

if (BUILD STREQUAL "pico")
    # if we're building for pico we need to do some setup before the project
    #set(PICO_BOARD mtm_computer_16mb CACHE STRING "Board type")
    set(PICO_BOARD pico CACHE STRING "Board type")
    set(PICO_SDK_FETCH_FROM_GIT on)
    include(pico_sdk_import.cmake)
    pico_sdk_init()
endif()

project(cosas VERSION 0.0 DESCRIPTION "ac's code for mtm" LANGUAGES CXX C)   # C for sdk - https://stackoverflow.com/a/74591212
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wdouble-promotion -Wfloat-conversion -Wall -Wextra -Werror)
add_link_options(-Wl,--print-memory-usage)

add_subdirectory(cosas/src)  # cosas_lib is common to both architectures

if (BUILD STREQUAL "pico")

    add_subdirectory(weas/src)  # weas_lib is pico-specific interface code

    # apps to deploy on cards
    add_subdirectory(apps/hola)
    add_subdirectory(apps/luz)
    #add_subdirectory(apps/knob)
    add_subdirectory(apps/diag)
    add_subdirectory(apps/dnl)
    add_subdirectory(apps/fifo)
    add_subdirectory(apps/fome)

else()

    # testing is only on laptop
    include(FetchContent)
    FetchContent_Declare(doctest GIT_REPOSITORY "https://github.com/onqtam/doctest" GIT_TAG "master")
    FetchContent_MakeAvailable(doctest)
    add_subdirectory(cosas/test)
    
    add_subdirectory(apps/dump)

endif()
