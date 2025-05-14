#!/bin/bash

set -x
rm -f *~
shopt -s extglob
rm -fr gcm.cache
GLOBIGNORE='main.cpp'
g++ -g -std=c++20 -fmodules -fsearch-include-path bits/std.cc *.cpp *.h
./a.out

