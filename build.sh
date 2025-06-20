#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

export PICO_SDK_PATH=`pwd`/pico-sdk
mkdir -p build
pushd build > /dev/null
cmake ..
