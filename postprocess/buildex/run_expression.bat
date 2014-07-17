@echo off

set CURRENT_DIR=%CD%

:: arguments
:: 1. trace file (to read the trace in)
:: 2. reverse file (to put the reverse in)
:: 3. expression file (to put the expression in)
:: 4. stdout redirection file (to put the stdout prints)
:: 5. 6. 7. arguments to the expression builder

:: workflow

:: 1. make reverse.exe
:: 2. first reverse the file -> reverse.exe <trace file> <reverse file>
:: 3. build the latest expression builder (32 bit build would be fine)
:: 4. invoke buildex.exe <reverse file> <expression file> <start_trace> <dest> <end_trace>

set argC=0
for %%x in (%*) do Set /A argC+=1

:: some file locations
set POST_PROCESS_FOLDER="C:\Charith\Dropbox\Research\development\exalgo\postprocess"
set BUILDEX_FOLDER="%POST_PROCESS_FOLDER%\buildex"

:: reverse the file

cd %BUILDEX_FOLDER%
reverse.exe %1 %2   

:: build the expression builder

if NOT EXIST build_32\NUL mkdir build_32
cd build_32
cmake -G"Visual Studio 12" -DDEBUG=ON ..
cmake --build . --config Debug

:: run the expression builder
cd bin

echo %argC%
if "%argC%" == "5"  buildex.exe %2 %3 %5 > %4
if "%argC%" == "7"  buildex.exe %2 %3 %5 %6 %7 > %4
if "%argC%" == "8"  buildex.exe %2 %3 %5 %6 %7 %8 > %4


cd %CURRENT_DIR%



