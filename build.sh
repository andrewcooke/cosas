#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")"

if [ $# -lt 1 ]; then
    echo "./build.sh (pico|amd64)"
    exit 1
fi

BUILD=$1
shift

pushd "build-$BUILD" > /dev/null
cmake --build . "$@"



