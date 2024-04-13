@echo off
rem dir "zlib-*" /B

cd 3rdparty
rd /S /Q _build
mkdir _build
cd _build

rem -DCMAKE_GENERATOR_PLATFORM=x64
rem cmake .. -G "Visual Studio 16 2019" -A Win32
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release --clean-first
cmake --build . --config Debug --clean-first
rem cmake --build . --target commonstatic --config Release --clean-first
rem cmake --build . --target commonstatic --config Debug --clean-first
rem cmake --build . --target commonstatic --config RelWithDebInfo --clean-first

rem forfiles /M *.dir /C "cmd /c if @isdir==TRUE rd /S /Q @file"

rem rd /S /Q CMakeFiles
rem rd /S /Q Win32

rem del /Q *.cmake
rem del /Q *.filters
rem del /Q *.vcxproj
rem del /Q *.sln
rem del /Q *.txt
rem del /Q *.pc
