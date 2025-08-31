#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

if [ $# -lt 1 ]; then
    echo "./build.sh (pico|amd64) [...]"
    echo "for example: ./build.sh pico --target luz"
    exit 1
fi

export BUILD=$1
shift

pushd "build-$BUILD" > /dev/null
cmake --build . "$@"



