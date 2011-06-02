#!/bin/sh
mkdir -p build/linux-x86-codeblocks
cd        build/linux-x86-codeblocks
# compil in 32 bits mode
export LDFLAGS=-m32
export CXXFLAGS=-m32
export CFLAGS=-m32
ccmake -G"CodeBlocks - Unix Makefiles" -DCMAKE_BUILD_ARCH=32 ../..
codeblocks docgl.cbp &
cd ../..
