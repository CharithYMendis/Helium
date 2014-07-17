@echo off

set CUR=%CD%
cd build_32\bin


if "%1"=="1" (
	
	buildex.exe C:\Charith\Dropbox\Research\development\exalgo\log\reverse_data.txt C:\Charith\Dropbox\Research\development\exalgo\log\expression_data.txt C:\Charith\Dropbox\Research\development\exalgo\log\halide_data.cpp


)

if "%1"=="2" (

	buildex.exe C:\Charith\Dropbox\Research\development\exalgo\log\reverse_blur.txt C:\Charith\Dropbox\Research\development\exalgo\log\expression_blur.txt C:\Charith\Dropbox\Research\development\exalgo\log\halide_blur.cpp 396403 5058816 400000

)


if "%1"=="3" (

	buildex.exe C:\Charith\Dropbox\Research\development\exalgo\log\reverse_blur.txt C:\Charith\Dropbox\Research\development\exalgo\log\expression_blur.txt C:\Charith\Dropbox\Research\development\exalgo\log\halide_blur.cpp

)


cd %CUR%