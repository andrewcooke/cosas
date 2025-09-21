#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

if [ $# -lt 1 ]; then
    echo "./build.sh (pico|esp32|amd64) [...]"
    echo "for example: ./build.sh pico --target luz"
    exit 1
fi

export BUILD=$1
shift

if [ $BUILD == "esp32" ]; then
    source esp-idf/export.sh
fi

pushd "build-$BUILD" > /dev/null
cmake --build . "$@"



