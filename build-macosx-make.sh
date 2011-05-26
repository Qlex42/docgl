#!/bin/sh
mkdir -p build/macosx-make/debug
cd        build/macosx-make/debug
../../../extern/bin/macosx/cmake/bin/ccmake -DCMAKE_BUILD_TYPE:String=Debug ../../..
make
cd ../../..
mkdir -p build/macosx-make/release
cd        build/macosx-make/release
../../../extern/bin/macosx/cmake/bin/ccmake -DCMAKE_BUILD_TYPE:String=Release ../../..
make
cd ../../..
