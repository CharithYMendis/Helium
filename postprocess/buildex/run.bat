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

set EXEC=


:: various runs based on options
set BASIC_TEST="C:\Charith\Dropbox\Research\development\exalgo\tests\basic_1.exe"

%DR_PATH% -debug -root %DYNAMORIO_HOME% -syntax_intel -c exalgo.dll -instrace 4 %LOG_DIR% hello.txt 300000 -- %BASIC_TEST%

:: %DR_PATH% -debug -root %DYNAMORIO_HOME% -logdir %LOG_DIR_DR% -msgbox_mask 0xf -- %BASIC_TEST%


cd %CURRENT_DIR%