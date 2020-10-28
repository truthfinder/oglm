@call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x64

@del *.exe
@cl /std:c++17 /O2 /arch:AVX /EHsc test.cpp > test.txt
@test.exe
@del *.obj
