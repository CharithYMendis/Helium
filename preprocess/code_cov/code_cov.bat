:: this is to get the code coverage results for photoshop
@echo off
set LOGDIR=C:\Charith\Dropbox\Research\development\exalgo\log
set DREXPORT=C:\Charith\libraries\dynamorio\exports
set PHOTOSHOP32="C:\Program Files (x86)\Adobe\Adobe Photoshop CS6\Photoshop.exe"
set PHOTOSHOP64="C:\Program Files\Adobe\Adobe Photoshop CS6 (64 Bit)\Photoshop.exe"

if "%1"=="m32" (
%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %PHOTOSHOP32%
)

if "%1"=="m64" (
%DREXPORT%\bin64\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %PHOTOSHOP64%
)
