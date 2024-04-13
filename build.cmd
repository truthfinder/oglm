@echo off

taskkill /IM PerfWatson2.exe /FI "STATUS eq RUNNING" /F

rd /S /Q _build
mkdir _build && cd _build

rem cmake .. -G "Visual Studio 16 2019" -A Win32
cmake .. -G "Visual Studio 17 2022" -A Win32

rem cmake --build . --target ALL_BUILD
rem devenv oglm.sln /build
rem cmake --build . -- -j4