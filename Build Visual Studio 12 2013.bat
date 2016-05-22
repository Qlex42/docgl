@echo off
if not exist build mkdir build
cd build
if not exist "Visual Studio 12 2013" mkdir "Visual Studio 12 2013"
cd "Visual Studio 12 2013"
if exist "C:\Program Files\Microsoft Visual Studio 12.0\Common7\Tools\" call "C:\Program Files\Microsoft Visual Studio 12.0\Common7\Tools\vsvars32.bat"
if exist "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\" call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\vsvars32.bat"
cmake-gui.exe ..\..
if exist "C:\Program Files\Microsoft Visual Studio 12.0\Common7\IDE\devenv.exe" call "C:\Program Files\Microsoft Visual Studio 12.0\Common7\IDE\devenv.exe" docgl.sln
if exist "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\devenv.exe" call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\IDE\devenv.exe" docgl.sln
cd ../..
