@echo off
if not exist build mkdir build
cd build
if not exist win32-msvc10 mkdir win32-msvc10
cd win32-msvc10
if exist "C:\Program Files\Microsoft Visual Studio 10.0\Common7\Tools\" call "C:\Program Files\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat"
if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\" call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\Tools\vsvars32.bat"
cmake-gui.exe ..\..
if exist "C:\Program Files\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" call "C:\Program Files\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" docgl.sln
if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE\devenv.exe" docgl.sln
cd ../..
