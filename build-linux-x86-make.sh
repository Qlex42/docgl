#!/bin/sh
# compil in 32 bits mode
export LDFLAGS=-m32
export CXXFLAGS=-m32
export CFLAGS=-m32
mkdir -p build/linux-x86-make/debug
cd        build/linux-x86-make/debug
../../../extern/bin/linux-x86/cmake/bin/ccmake -DCMAKE_BUILD_TYPE:String=Debug -DCMAKE_BUILD_ARCH=32 ../../..
make
cd ../../..
mkdir -p build/linux-x86-make/release
cd        build/linux-x86-make/release
../../../extern/bin/linux-x86/cmake/bin/ccmake -DCMAKE_BUILD_TYPE:String=Release -DCMAKE_BUILD_ARCH=32 ../../..
make
cd ../../..
