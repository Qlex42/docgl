#!/bin/sh
mkdir -p "build/KDevelop3 Linux32"
cd        "build/KDevelop3 Linux32"
# compil in 32 bits mode
export LDFLAGS=-m32
export CXXFLAGS=-m32
export CFLAGS=-m32
ccmake -G"KDevelop3" ../..
kdevelop docgl.kdevelop &
cd ../..
