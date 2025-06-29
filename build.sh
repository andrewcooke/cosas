#!/bin/bash -x

cd "$(dirname "${BASH_SOURCE[0]}")"

pushd build > /dev/null
cmake --build . "$@"


