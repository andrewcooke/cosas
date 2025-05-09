#!/bin/bash

rm -fr gcm.cache
g++ -g -std=c++20 -fmodules -fsearch-include-path bits/std.cc *.cpp *.h

