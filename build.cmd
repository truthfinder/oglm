@echo off

taskkill /IM PerfWatson2.exe /FI "STATUS eq RUNNING" /F

rd /S /Q _build
mkdir _build && cd _build

cmake .. -G "Visual Studio 15 2017"

rem cmake --build . --target ALL_BUILD
rem devenv oglm.slm /build
rem cmale --build . -- -j4