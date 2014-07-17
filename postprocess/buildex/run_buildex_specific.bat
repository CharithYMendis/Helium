@echo off

set CUR=%CD%
cd build_32\bin


if "%1"=="1" (
	
	buildex.exe C:\Charith\Dropbox\Research\development\exalgo\log reverse_data expression_data halide_data


)

if "%1"=="2" (

	buildex.exe C:\Charith\Dropbox\Research\development\exalgo\log reverse_blur expression_blur halide_blur 396403 5058816 400000

)


if "%1"=="3" (

	buildex.exe C:\Charith\Dropbox\Research\development\exalgo\log reverse_blur expression_blur halide_blur

)


cd %CUR%