:: this is to get the code coverage results for photoshop
@echo off
set LOGDIR=%EXALGO_OUTPUT_FOLDER%
set DREXPORT=%DYNAMORIO_HOME%\exports

if "%1"=="m32" (
 	%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %2
)

if "%1"=="m64" (
	%DREXPORT%\bin64\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %2
)



