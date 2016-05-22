@echo off
if not exist build mkdir build
cd build
if not exist "Visual Studio 12 2013 Win64" mkdir "Visual Studio 12 2013 Win64"
cd "Visual Studio 12 2013 Win64"
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
cmake-gui.exe ..\..
if exist docgl.sln start /MAX Devenv docgl.sln
cd ../..
