:: build the entire project
@echo off

if "%1" == "help" (
	echo {m32/m64} {debug/release} {library}
	exit /b
)

set ARCH=%1
set MODE=%2
set LIBRARY=%3

set CURRENT_DIR=%CD%

if NOT EXIST %LIBRARY%\NUL (
	mkdir %LIBRARY%
)

set HALIDE_DIR=halide_build


:: build dynamorio
call dr_build %LIBRARY% dynamorio

:: build halide
if "%ARCH%" == "m32" (
	powershell.exe -executionpolicy remotesigned -File halide_32_build -library %LIBRARY%
)

if "%ARCH%" == "m64" (
	powershell.exe -executionpolicy remotesigned -File halide_64_build -library %LIBRARY% -halide %HALIDE_DIR%
)

:: build project
call build_all_exalgo %ARCH% %MODE% %LIBRARY%\%HALIDE_DIR%

cd %CURRENT_DIR%