@echo off

if "%1" == "help" (
	echo usage {mode} {exec} {images}...  
	echo {mode} - diff, oneim, twoim
	echo {exec} - exexutable on which the DR client was run 
	echo {images} - space seperated list of images
	exit /b
)



set CURRENT_DIR=%CD%
					
set MODE=%1
set EXEC=%2
set IMAGE_1=%3
set IMAGE_2=%4
					

set argC=0
for %%x in (%*) do Set /A argC+=1								

cd ..\build32\bin

if "%MODE%" == "diff" (
	filter.exe %MODE% %EXEC% %IMAGE_1%
)

if "%MODE%" == "oneim" (
	filter.exe %MODE% %EXEC% %IMAGE_1%
)

if "%MODE% == "twoim" (
	buildex.exe %MODE% %EXEC% %IMAGE_1% %IMAGE_2%
)

cd %CURRENT_DIR%