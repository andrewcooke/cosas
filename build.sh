#!/bin/bash -x

cd "$(dirname "${BASH_SOURCE[0]}")"

if [ ! -e pico_sdk_import.cmake ]; then
    wget -L https://raw.github.com/raspberrypi/pico-sdk/master/pico_sdk_import.cmake
fi

mkdir -p build
pushd build > /dev/null
cmake ..
make
