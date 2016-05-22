@echo off
if not exist build mkdir build
cd build
if not exist "Visual Studio 11 2012 Win64" mkdir "Visual Studio 11 2012 Win64"
cd "Visual Studio 11 2012 Win64"
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" x64
cmake-gui.exe ..\..
if exist docgl.sln start /MAX Devenv docgl.sln
cd ../..
