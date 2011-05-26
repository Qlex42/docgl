@echo off
if not exist build mkdir build
cd build
if not exist win32-msvc10-express mkdir win32-msvc10-express
cd win32-msvc10-express
call "C:\Program Files\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat"
cmake-gui.exe ..\..
if exist docgl.sln start /MAX VCExpress docgl.sln
cd ../..
