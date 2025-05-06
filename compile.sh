#!/bin/bash

g++ -std=c++20 -fmodules -fsearch-include-path bits/std.cc *.cpp *.h

