@echo off
if not exist build mkdir build
cd build
if not exist "MinGW Makefile" mkdir "MinGW Makefile"
cd "MinGW Makefile"
cmake-gui.exe ..\..
if exist "Makefile" call "mingw32-make"
pause
cd ../..
