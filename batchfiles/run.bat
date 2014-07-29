@echo off

:: arguments
:: bbinfo
:: folder ,  in_filename , out_filename , summary_filename , threshold , mode
:: cpuid
:: memtrace
:: inscount


:: general main arguments
:: -bbinfo %LOG_DIR% diff freq summary 0 2
									   


set LOG_DIR=C:\Charith\Dropbox\Research\development\exalgo\log\
set LOG_DIR_DR=C:\Charith\Dropbox\Research\development\exalgo\log
set CURRENT_DIR=%CD%

if "%1"=="m64" (
cd ..\build\bin
set DR_PATH=%DYNAMORIO_64_DEBUG_HOME%\bin64\drrun.exe
set DYNAMORIO_HOME=%DYNAMORIO_64_DEBUG_HOME%
set PHOTOSHOP="C:\Program Files\Adobe\Adobe Photoshop CS6 (64 Bit)\Photoshop.exe"
)

if "%1"=="m32" (
cd ..\build_32\bin
set DR_PATH=%DYNAMORIO_32_DEBUG_HOME%\bin32\drrun.exe
set DYNAMORIO_HOME=%DYNAMORIO_32_DEBUG_HOME%
set PHOTOSHOP="C:\Program Files (x86)\Adobe\Adobe Photoshop CS6\Photoshop.exe"
)

:: various runs based on options
set BASIC_TEST="C:\Charith\Dropbox\Research\development\exalgo\tests\c_tests\output\image_blur.exe"
set ASM_TEST="C:\Charith\Dropbox\Research\development\exalgo\tests\asm_1.exe"
set HALIDE_TEST="C:\Charith\Dropbox\Research\development\exalgo\tests\halide_tests\output_tests_x86\halide_blur_test.exe"
set FILTER_FILE="C:\Charith\Dropbox\Research\development\exalgo\tests\c_tests\filter_files\image_blur_filter.txt"

set HALIDE_FILTER_FILE="C:\Charith\Dropbox\Research\development\exalgo\tests\halide_tests\filter.txt"

set INPUT_IMAGE="C:\Charith\Dropbox\Research\development\exalgo\tests\images\forty.png"
set OUTPUT_IMAGE="C:\Charith\Dropbox\Research\development\exalgo\tests\images\output.png"

:: %DR_PATH% -debug -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll -instrace 4 %LOG_DIR% hello.txt 300000 -- %ASM_TEST%
:: -loglevel 3 -logdir %LOG_DIR_DR%
:: %DR_PATH% -debug -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll -inscount %FILTER_FILE% 3 -- %BASIC_TEST% %INPUT_IMAGE% %OUTPUT_IMAGE%

:: %DR_PATH% -debug -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll -instrace 3 %LOG_DIR% %FILTER_FILE% 300000 -- %BASIC_TEST% %INPUT_IMAGE% %OUTPUT_IMAGE%

%DR_PATH% -debug -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll -bbinfo %LOG_DIR% input output summary 0 6 -- %HALIDE_TEST% %INPUT_IMAGE% %OUTPUT_IMAGE%

:: -instrace 3 %LOG_DIR% %FILTER_FILE% 300000

:: -syntax_intel -c exalgo.dll -instrace 3 %LOG_DIR% %FILTER_FILE% 300000 -- %BASIC_TEST%

:: %DR_PATH% -debug -root %DYNAMORIO_HOME% -logdir %LOG_DIR_DR% -msgbox_mask 0xf -- %BASIC_TEST%


cd %CURRENT_DIR%