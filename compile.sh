#!/bin/bash

set -e
set -x
rm -f *~
shopt -s extglob
GLOBIGNORE='test.cpp'
g++ -DDOCTEST_CONFIG_DISABLE -g -std=c++20 *.cpp

