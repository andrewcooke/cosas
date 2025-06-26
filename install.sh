#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib g++

if [ ! -e pico_sdk_import.cmake ]; then
    wget -L https://raw.github.com/raspberrypi/pico-sdk/master/external/pico_sdk_import.cmake
fi

mkdir -p build
pushd build > /dev/null
cmake ..
