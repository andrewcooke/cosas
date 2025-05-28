#!/bin/bash

set -e
set -x
rm -f *~
shopt -s extglob
GLOBIGNORE='main.cpp'
# https://stackoverflow.com/a/9862800
g++ -DDOCTEST_CONFIG_REQUIRE_STRINGIFICATION_FOR_ALL_USED_TYPES -g -Wall -Werror -pedantic -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -std=c++20 *.cpp
./a.out $@

