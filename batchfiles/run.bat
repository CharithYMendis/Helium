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

%DR_PATH% -debug -root %DYNAMORIO_HOME% -c exalgo.dll -inscount -- %PHOTOSHOP%

cd %CURRENT_DIR%