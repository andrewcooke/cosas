#!/bin/bash -x

git config --global http.postBuffer 524288000

cd "$(dirname "${BASH_SOURCE[0]}")"

# pico
sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib g++ picotool gcc-multilib
# esp32
sudo apt install -y git wget flex bison gperf python3 python3-venv cmake ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

if [ ! -e pico_sdk_import.cmake ]; then
    wget -L https://raw.github.com/raspberrypi/pico-sdk/master/external/pico_sdk_import.cmake
fi

if [ ! -e esp-idf ]; then
    git clone --recursive https://github.com/espressif/esp-idf.git
fi
esp-idf/install.sh
source esp-idf/export.sh

mkdir -p build-esp32
pushd build-esp32 > /dev/null
BUILD=esp32 cmake -DBUILD=esp32 ..
popd > /dev/null

mkdir -p build-pico
pushd build-pico > /dev/null
BUILD=pico cmake -DBUILD=pico ..
popd > /dev/null

mkdir -p build-amd64
pushd build-amd64 > /dev/null
BUILD=amd64 cmake -DBUILD=amd64 ..
popd > /dev/null

