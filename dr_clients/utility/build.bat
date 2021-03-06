:: hardcoded for client genie 

@echo off

set CURRENT_DIR=%CD%

:: select 32 bit or 64 bit
:: first, create the necessary directories if they do not exist and then cmake the project and build it

if "%1"=="m64" (
if NOT EXIST ..\build64\NUL mkdir ..\build64
cd ..\build64
if EXIST CMakeCache.txt del CMakeCache.txt
	if "%2"=="debug" (
		cmake -G"Visual Studio 12 Win64" -DDynamoRIO_DIR=%DYNAMORIO_64_DEBUG_HOME%\cmake -DDEBUG=ON ..
	)
	if "%2"=="release" (
		cmake -G"Visual Studio 12 Win64" -DDynamoRIO_DIR=%DYNAMORIO_64_RELEASE_HOME%\cmake ..
	)
)

if "%1"=="m32" (
if NOT EXIST ..\build_32\NUL mkdir ..\build32
cd ..\build32
if EXIST CMakeCache.txt del CMakeCache.txt
	if "%2"=="debug" (
		cmake -G"Visual Studio 12" -DDynamoRIO_DIR=%DYNAMORIO_32_DEBUG_HOME%\cmake -DDEBUG=ON ..
	)
	if "%2"=="release" (
		cmake -G"Visual Studio 12" -DDynamoRIO_DIR=%DYNAMORIO_32_RELEASE_HOME%\cmake ..
	)
)

if "%2"=="debug" (
	cmake --build . --config Debug
)
if "%2"=="release" (
	cmake --build . --config Release
)

cd %CURRENT_DIR%



