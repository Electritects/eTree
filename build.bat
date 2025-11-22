@echo off
if exist etree.exe del etree.exe
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
cl /std:c++17 /EHsc /W3 /O2 /D_CRT_SECURE_NO_WARNINGS /Fe:etree.exe main.cpp etree.cpp args.cpp help.cpp csv.cpp
if %errorlevel% neq 0 exit /b %errorlevel%
echo Build Successful
