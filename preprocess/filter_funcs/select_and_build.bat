@echo off

set CURRENT_DIR=%CD%

:: select 32 bit or 64 bit
:: first, create the necessary directories if they do not exist and then cmake the project and build it

if "%1"=="m64" (
if NOT EXIST build\NUL mkdir build
cd build
if EXIST CMakeCache.txt del CMakeCache.txt
	if "%2"=="debug" (
		cmake -G"Visual Studio 12 Win64" -DDEBUG=ON ..
	)
	if "%2"=="release" (
		cmake -G"Visual Studio 12 Win64" ..
	)
)

if "%1"=="m32" (
if NOT EXIST build_32\NUL mkdir build_32
cd build_32
if EXIST CMakeCache.txt del CMakeCache.txt
	if "%2"=="debug" (
		cmake -G"Visual Studio 12" -DDEBUG=ON ..
	)
	if "%2"=="release" (
		cmake -G"Visual Studio 12" ..
	)
)

if "%2"=="debug" (
	cmake --build . --config Debug
)
if "%2"=="release" (
	cmake --build . --config Release
)

cd %CURRENT_DIR%



