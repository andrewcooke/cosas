#!/bin/bash

set -x
rm -f *~
shopt -s extglob
rm -fr gcm.cache
GLOBIGNORE='test.cpp'
g++ -DDOCTEST_CONFIG_DISABLE -g -std=c++20 *.cpp

