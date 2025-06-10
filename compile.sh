#!/bin/bash

set -e
set -x
rm -f *~
shopt -s extglob
GLOBIGNORE='test.cpp'
#g++ -DDOCTEST_CONFIG_DISABLE -g -std=c++20 -fext-numeric-literals *.cpp
# https://stackoverflow.com/questions/15314581/g-compiler-flag-to-minimize-binary-size
g++ -DDOCTEST_CONFIG_DISABLE -std=c++20 -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -fext-numeric-literals *.cpp

