@echo off
rem dir "zlib-*" /B

cd 3rdparty/zlib
rd /S /Q _build
mkdir _build
cd _build

cmake .. -G "Visual Studio 15 2017"
cmake --build . --target zlibstatic --config Release --clean-first
cmake --build . --target zlibstatic --config Debug --clean-first
cmake --build . --target zlibstatic --config RelWithDebInfo --clean-first

forfiles /M *.dir /C "cmd /c if @isdir==TRUE rd /S /Q @file"

rd /S /Q CMakeFiles
rd /S /Q Win32

del /Q *.cmake
del /Q *.filters
del /Q *.vcxproj
del /Q *.sln
del /Q *.txt
del /Q *.pc
