#!/bin/bash

set -x
rm -f *~
shopt -s extglob
rm -fr gcm.cache
GLOBIGNORE='main.cpp'
g++ -DDOCTEST_CONFIG_REQUIRE_STRINGIFICATION_FOR_ALL_USED_TYPES -g -std=c++20 -fmodules -fsearch-include-path bits/std.cc *.cpp
./a.out

