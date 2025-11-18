#!/bin/sh

rm -vf prog5.x86_64
rm -rvf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cp build/prog5 ./prog5.x86_64
