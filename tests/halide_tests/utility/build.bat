@echo off
Setlocal EnableDelayedExpansion

set CURRENT_DIR=%CD%

:: select 32 bit or 64 bit
:: first, create the necessary directories if they do not exist and then cmake the project and build it

set halide_target=halide_blur;halide_rotate;halide_input_dep;
set test_target=

for %%s in (%halide_target%) do (
	set test_target=!test_target!;%%s_test
)

if "%1"=="m64" (
if NOT EXIST ..\build64\NUL mkdir ..\build64
cd ..\build64
if EXIST CMakeCache.txt del CMakeCache.txt
if "%2"=="1" (
cmake -G"Visual Studio 12 Win64" -DTYPE=x64 -DFIRST=1 .. 
)
if "%2"=="0" (
cmake -G"Visual Studio 12 Win64" -DTYPE=x64 ..
)
)

if "%1"=="m32" (
if NOT EXIST ..\build32\NUL mkdir ..\build32
cd ..\build32
if EXIST CMakeCache.txt del CMakeCache.txt
if "%2"=="1" (
cmake -G"Visual Studio 12" -DTYPE=x86 -DFIRST=1 ..
)
if "%2"=="0" (
cmake -G"Visual Studio 12" -DTYPE=x86 ..
)
)

for %%t in (%halide_target%) do (
	cmake --build . --config Release --target %%t
)

for %%t in (%test_target%) do (
	cmake --build . --config Debug --target %%t
)

cd %CURRENT_DIR%



