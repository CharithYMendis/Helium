@echo off

if "%1" == "help" (
	echo usage {first} {second} {exec}
	echo {first} - first drcov file
	echo {second} - second drcov file
	echo {exec} - this is the executable that was run to get the code coverage files
	exit /b
)

set CURRENT_DIR=%CD%
					
set FIRST=%1
set SECOND=%2
set EXEC=%3
			
cd ..\build32\bin
			
code_diff.exe %FIRST% %SECOND% %EXEC%

cd %CURRENT_DIR%