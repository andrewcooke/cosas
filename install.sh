#!/bin/bash -x

cd "$(dirname "${BASH_SOURCE[0]}")"

sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib g++ picotool cpputest gcc-multilib

if [ ! -e pico_sdk_import.cmake ]; then
    wget -L https://raw.github.com/raspberrypi/pico-sdk/master/external/pico_sdk_import.cmake
fi

mkdir -p build-pico
pushd build-pico > /dev/null
cmake -DBUILD=pico ..
popd > /dev/null

mkdir -p build-amd64
pushd build-amd64 > /dev/null
cmake -DBUILD=amd64 ..
popd > /dev/null
