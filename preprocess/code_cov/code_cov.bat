:: this is to get the code coverage results for photoshop
@echo off
set LOGDIR=%EXALGO_OUTPUT_FOLDER%
set DREXPORT=%DYNAMORIO_HOME%\exports
set PHOTOSHOP32="C:\Program Files (x86)\Adobe\Adobe Photoshop CS6\Photoshop.exe"
set PHOTOSHOP64="C:\Program Files\Adobe\Adobe Photoshop CS6 (64 Bit)\Photoshop.exe"
set GIMP="C:\Program Files\GIMP 2\bin\gimp-2.8.exe"

if "%1"=="m32" (
%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %PHOTOSHOP32%
)

if "%1"=="m64" (
%DREXPORT%\bin64\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %PHOTOSHOP64%
)

if "%1"=="gimp" (
%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %GIMP%
)


