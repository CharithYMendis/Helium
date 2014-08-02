@echo off

:: functrace - filter_file filter_mode
:: profile - filter_file out_file summary_file threshold filter_mode
:: memtrace - filter_file out_file filter_mode

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


set ASM_TEST="C:\Charith\Dropbox\Research\development\exalgo\tests\asm_1.exe"
set BASIC_TEST="C:\Charith\Dropbox\Research\development\exalgo\tests\c_tests\output\image_blur.exe"

set HALIDE_TEST="C:\Charith\Dropbox\Research\development\exalgo\tests\halide_tests\output_tests_x86\%2.exe"
set INPUT_IMAGE="C:\Charith\Dropbox\Research\development\exalgo\tests\images\forty.png"
set OUTPUT_IMAGE="C:\Charith\Dropbox\Research\development\exalgo\tests\images\output.png"


:: path locations
set PREPROCESS_FOLDER=C:\Charith\Dropbox\Research\development\exalgo\preprocess
set IN_FOLDER=%PREPROCESS_FOLDER%\in_files
set OUT_FOLDER=%PREPROCESS_FOLDER%\out_files

set OUT_EXT=txt
set PROFILE_PREFIX=profile
set SUMMARY_PREFIX=summary
set MEMTRACE_PREFIX=memtrace

set FILTER_FILE=%IN_FOLDER%\filter_file.txt
set PROFILE_FILE=%OUT_FOLDER%\%PROFILE_PREFIX%_%2.%OUT_EXT%
set SUMMARY_FILE=%OUT_FOLDER%\%SUMMARY_PREFIX%_%2.%OUT_EXT%
set MEMTRACE_FILE=%OUT_FOLDER%\%MEMTRACE_PREFIX%_%2

del %MEMTRACE_FILE%*

%DR_PATH% -debug -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll -functrace %FILTER_FILE% 5 -bbinfo %FILTER_FILE% %PROFILE_FILE% %SUMMARY_FILE% 10 5 -memtrace %FILTER_FILE% %MEMTRACE_FILE% 5 -- %HALIDE_TEST% %INPUT_IMAGE% %OUTPUT_IMAGE%

cd %CURRENT_DIR%