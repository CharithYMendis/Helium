@echo off

if "%1"=="help" (
	echo usage {arch} {exec} {client_arg} {test_name}
	echo 	{arch} m32, m64
	echo 	{exec} photoshop, halide, ctests, asm
	echo 	{client_arg}
	echo 	{test_name}
	exit /b
)


set CURRENT_DIR=%CD%

:: you could either run photoshop, halide_tests, c_tests or asm_tests

:: setup paths for various machine architectures
if "%1"=="m64" (
cd ..\build64\bin
set DR_PATH=%DYNAMORIO_64_DEBUG_HOME%\bin64\drrun.exe
set DYNAMORIO_HOME=%DYNAMORIO_64_DEBUG_HOME%
set PHOTOSHOP="C:\Program Files\Adobe\Adobe Photoshop CS6 (64 Bit)\Photoshop.exe"
set HALIDE_TEST=%EXALGO_TEST_FOLDER%\halide_tests\bin64\%4.exe
set ASM_TEST=%EXALGO_TEST_FOLDER%\asm_tests\bin64\%4.exe
set C_TEST=%EXALGO_TEST_FOLDER%\c_tests\bin64\%4.exe
)

if "%1"=="m32" (
cd ..\build32\bin
set DR_PATH=%DYNAMORIO_32_RELEASE_HOME%\bin32\drrun.exe
set DYNAMORIO_HOME=%DYNAMORIO_32_RELEASE_HOME%
set PHOTOSHOP="C:\Program Files (x86)\Adobe\Adobe Photoshop CS6\Photoshop.exe"
set HALIDE_TEST=%EXALGO_TEST_FOLDER%\halide_tests\bin32\%4.exe
set ASM_TEST=%EXALGO_TEST_FOLDER%\asm_tests\bin32\%4.exe
set C_TEST=%EXALGO_TEST_FOLDER%\c_tests\bin32\%4.exe
)

set INPUT_IMAGE=%EXALGO_IMAGE_FOLDER%\%IN_IMAGE%
set OUTPUT_IMAGE=%EXALGO_IMAGE_FOLDER%\%OUT_IMAGE%

set OUTPUT_FOLDER=%EXALGO_OUTPUT_FOLDER%

if "%2" == "photoshop" (
	set EXEC=%PHOTOSHOP%
) 
if "%2" == "halide" (
	set EXEC=%HALIDE_TEST% %INPUT_IMAGE% %OUTPUT_IMAGE%
)
if "%2" == "ctest" (
	set EXEC=%C_TEST% %INPUT_IMAGE% %OUTPUT_IMAGE%
)
if "%2" == "asm" (
	set EXEC=%ASM_TEST%
)
 
 ::-msgbox_mask 0xf
%DR_PATH% -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll %3 -- %EXEC%

cd %CURRENT_DIR%