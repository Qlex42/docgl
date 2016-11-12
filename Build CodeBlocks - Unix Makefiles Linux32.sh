#!/bin/sh
mkdir -p "build/CodeBlocks - Unix Makefiles Linux32"
cd        "build/CodeBlocks - Unix Makefiles Linux32"
# compil in 32 bits mode
export LDFLAGS=-m32
export CXXFLAGS=-m32
export CFLAGS=-m32
ccmake -G"CodeBlocks - Unix Makefiles" ../..
codeblocks docgl.cbp &
cd ../..
