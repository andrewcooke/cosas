#!/bin/bash -x

cd "$(dirname "${BASH_SOURCE[0]}")"

export PICO_SDK_PATH=`pwd`/pico-sdk

#rm -fr build

mkdir -p build
pushd build > /dev/null
cmake ..
make
