#! /bin/bash

mkdir -p /ignis/build
cd /ignis/build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DAnyDSL_runtime_DIR=/anydsl/runtime/build/share/anydsl/cmake ..
cmake --build .
cmake --install .
ninja clean
