#!/bin/sh
mkdir -p build/macosx-xcode
cd        build/macosx-xcode
../../extern/bin/macosx/cmake/bin/ccmake -G"Xcode" ../..
# xcodebuild -alltargets
open docgl.xcodeproj
cd ../..
