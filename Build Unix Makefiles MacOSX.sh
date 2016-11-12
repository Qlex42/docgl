#!/bin/sh
mkdir -p "build/Unix Makefiles MacOSX/debug"
cd        "build/Unix Makefiles MacOSX/debug"
../../../extern/bin/macosx/cmake/bin/ccmake -DCMAKE_BUILD_TYPE:String=Debug ../../..
make
cd ../../..
mkdir -p "build/Unix Makefiles MacOSX/release"
cd        "build/Unix Makefiles MacOSX/release"
../../../extern/bin/macosx/cmake/bin/ccmake -DCMAKE_BUILD_TYPE:String=Release ../../..
make
cd ../../..
