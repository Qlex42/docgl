@echo off
if not exist build mkdir build
cd build
if not exist win32-msvc9-express mkdir win32-msvc9-express
cd win32-msvc9-express
call "C:\Program Files\Microsoft Visual Studio 9.0\Common7\Tools\vsvars32.bat"
cmake-gui.exe ..\..
if exist docgl.sln start /MAX VCExpress docgl.sln
cd ../..
