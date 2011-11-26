#!/bin/sh
# compil in 32 bits mode
export LDFLAGS=-m32
export CXXFLAGS=-m32
export CFLAGS=-m32
mkdir -p build/linux-x86-make/debug
cd        build/linux-x86-make/debug
ccmake -DCMAKE_BUILD_TYPE:String=Debug ../../..
make
cd ../../..
mkdir -p build/linux-x86-make/release
cd        build/linux-x86-make/release
ccmake -DCMAKE_BUILD_TYPE:String=Release ../../..
make
cd ../../..
