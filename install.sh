#!/bin/bash

sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib g++
git clone https://github.com/raspberrypi/pico-sdk.git
pushd pico-sdk > /dev/null
git submodule update --init
popd
ln -s pico-sdk/external/pico_sdk_import.cmake .
