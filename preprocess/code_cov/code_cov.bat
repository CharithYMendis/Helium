:: this is to get the code coverage results for photoshop
@echo off
set LOGDIR=%EXALGO_OUTPUT_FOLDER%
set DREXPORT=%DYNAMORIO_HOME%\exports
set PHOTOSHOP32="C:\Program Files (x86)\Adobe\Adobe Photoshop CS6\Photoshop.exe"
set PHOTOSHOP64="C:\Program Files\Adobe\Adobe Photoshop CS6 (64 Bit)\Photoshop.exe"
set GIMP="C:\Program Files\GIMP 2\bin\gimp_2.8.exe"
set PPT="C:\Program Files (x86)\Microsoft Office\Office15\POWERPNT.EXE"
set IRFAN="C:\Program Files (x86)\IrfanView\i_view32.exe"
set HPGMG="C:\Charith\libraries\hpgmg\build\bin\hpgmg_fv.exe"
set MINIGMG="C:\Charith\libraries\minigmg\miniGMG\run.exe"



if "%1"=="m32" (
 if "%2"=="photoshop" (
	%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %PHOTOSHOP32%
 )
 if "%2"=="gimp" (
	%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %GIMP%
 )
 if "%2"=="ppt" (
	echo %PPT%
	%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %PPT%
 )
 if "%2"=="irfan" (
	%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %IRFAN%
 )
 
 if "%2"=="hpgmg" (
	%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %HPGMG%
 )
 
 if "%2"=="minigmg" (
	%DREXPORT%\bin32\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %MINIGMG%
 )
 
)

if "%1"=="m64" (
 if "%2"=="photoshop" (
	%DREXPORT%\bin64\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %PHOTOSHOP64%
 )
 if "%2"=="gimp" (
	%DREXPORT%\bin64\drrun.exe -root %DREXPORT% -t drcov -dump_text -logdir %LOGDIR% -- %GIMP%
 )
)



