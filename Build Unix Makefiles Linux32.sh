#!/bin/sh
# compil in 32 bits mode
export LDFLAGS=-m32
export CXXFLAGS=-m32
export CFLAGS=-m32
mkdir -p "build/Unix Makefiles Linux32/debug"
cd        "build/Unix Makefiles Linux32/debug"
ccmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE:String=Debug ../../.. 
make -j 8
cd ../../..
mkdir -p "build/Unix Makefiles Linux32/release"
cd       "build/Unix Makefiles Linux32/release"
ccmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE:String=Release ../../..
make -j 8
cd ../../..
