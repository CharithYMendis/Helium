@echo off

if "%1"=="help" (
	echo usage {arch} {exec} {client_arg} {test_name}
	echo 	{arch} m32, m64
	echo 	{exec} photoshop, halide, ctests, asm, gimp, ppt
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
set HALIDE_TEST=%EXALGO_TEST_FOLDER%\halide_tests\bin64\%4
set ASM_TEST=%EXALGO_TEST_FOLDER%\asm_tests\bin64\%4
set C_TEST=%EXALGO_TEST_FOLDER%\c_tests\bin64\%4
)

if "%1"=="m32" (
cd ..\build32\bin
set DR_PATH=%DYNAMORIO_32_RELEASE_HOME%\bin32\drrun.exe
set DYNAMORIO_HOME=%DYNAMORIO_32_RELEASE_HOME%
set PHOTOSHOP="C:\Program Files (x86)\Adobe\Adobe Photoshop CS6\Photoshop.exe"
set HALIDE_TEST=%EXALGO_TEST_FOLDER%\halide_tests\bin32\%4
set ASM_TEST=%EXALGO_TEST_FOLDER%\asm_tests\bin32\%4
set C_TEST=%EXALGO_TEST_FOLDER%\c_tests\bin32\%4
set GIMP="C:\Program Files\GIMP 2\bin\gimp_2.8.exe"
set PPT="C:\Program Files (x86)\Microsoft Office\Office15\POWERPNT.EXE"
set IRFAN="C:\Program Files (x86)\IrfanView\i_view32.exe"
set HPGMG="C:\Charith\libraries\hpgmg\build\bin\hpgmg_fv.exe"
set MINIGMG="C:\Charith\libraries\minigmg\miniGMG\run.exe"
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
if "%2" == "gimp" (
	set EXEC=%GIMP%
)

if "%2" == "ppt" (
	set EXEC=%PPT%
)

if "%2" == "hpgmg" (
	set EXEC=%HPGMG%
)

if "%2" == "irfan" (
	set EXEC=%IRFAN%
)

if "%2" == "minigmg" (
	set EXEC=%MINIGMG%
)
 
 ::-msgbox_mask 0xf
 echo %DR_PATH% -root %DYNAMORIO_HOME% -debug -syntax_intel -thread_private -c exalgo.dll %3 -- %EXEC%
%DR_PATH% -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll %3 -- %EXEC%

cd %CURRENT_DIR%