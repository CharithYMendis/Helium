:: to build the complete suite of DR using this bat fill

:: library locations
@echo off

if "%1" == "help" (
	echo {library} {dr_folder_name}
	exit /b
)

set CURRENT_DIR=%CD%

if "%1" == "" (
	echo please give out library directory..
	exit /b
)

set DR_HOME=%1\%2
if "%2" == "" (
	set DR_HOME=%1\dynamorio
)

if NOT EXIST %DR_HOME%\NUL (
	mkdir %DR_HOME%
	cd %DR_HOME%
	svn co -r2773 http://dynamorio.googlecode.com/svn/trunk/ .
)

:: setting up dynamorio environment variables
setx DYNAMORIO_32_DEBUG_HOME %DR_HOME%\build_debug_32
setx DYNAMORIO_32_RELEASE_HOME %DR_HOME%\build_release_32
setx DYNAMORIO_64_DEBUG_HOME %DR_HOME%\build_debug_64
setx DYNAMORIO_64_RELEASE_HOME %DR_HOME%\build_release_64
setx DYNAMORIO_HOME %DR_HOME%

:: set up the necessary environments - 32 bit
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\vcvars32.bat"
cd %DR_HOME%

:: update svn
svn up -r2773

:: building 32 bit debug 
if NOT EXIST build_debug_32\NUL mkdir build_debug_32
cd build_debug_32
cmake -G"Visual Studio 12" .. -DDEBUG=ON
cmake --build . --config Debug --target install

cmake -G"Visual Studio 12" .. -DDEBUG=ON
cmake --build . --config Debug
echo debug_32_done 1>&2
cd %DR_HOME%

:: building 32 bit release + install
if NOT EXIST build_release_32\NUL mkdir build_release_32
cd build_release_32
cmake -G"Visual Studio 12" ..
cmake --build . --config RelWithDebInfo --target install

cmake -G"Visual Studio 12" ..
cmake --build . --config RelWithDebInfo
echo release_32_done 1>&2

:: set up the necessary environments - 64 bit
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64\vcvars64.bat"
cd %DR_HOME%

:: building 64 bit debug 
if NOT EXIST build_debug_64\NUL mkdir build_debug_64
cd build_debug_64
cmake -G"Visual Studio 12 Win64" .. -DDEBUG=ON
cmake --build . --config Debug --target install

cmake -G"Visual Studio 12 Win64" .. -DDEBUG=ON
cmake --build . --config Debug
echo debug_64_done 1>&2
cd %DR_HOME%

:: building 32 bit release + install
if NOT EXIST build_release_64\NUL mkdir build_release_64
cd build_release_64
cmake -G"Visual Studio 12 Win64" ..
cmake --build . --config RelWithDebInfo --target install

cmake -G"Visual Studio 12 Win64" ..
cmake --build . --config RelWithDebInfo
echo release_64_done 1>&2

cd %CURRENT_DIR%