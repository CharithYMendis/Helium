@echo off

if "%1" == "help" (
	echo usage {exec} {start_line} {dest} {end_line} 
	echo {exec} - the executable on which DR was run to get the instrace
	echo {start_line} - starting line (optional)
	echo {dest} - destination address (optional)
	echo {end_line} - ending line (optional)
	exit /b
)

set CURRENT_DIR=%CD%
					
set EXEC=%1
set START=%2
set END=%3
set DEST=%4
					

set argC=0
for %%x in (%*) do Set /A argC+=1								

cd ..\build32\bin

if "%argC%" == "1" (
	buildex.exe %EXEC%
)

if "%argC%" == "3" (
	buildex.exe %EXEC% %START% %DEST%
)

if "%argC% == "4" (
	buildex.exe %EXEC% %START% %DEST% %END%
)

cd %CURRENT_DIR%