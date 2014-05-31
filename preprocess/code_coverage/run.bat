:: building the my diff utility and running it
@echo off
set LOGDIR=C:\Charith\Dropbox\Research\development\photo\logs

set FIRST=%LOGDIR%\%1
set SECOND=%LOGDIR%\%2
set OUT=%LOGDIR%\%3

:: build it
g++ -o mydiff diff.cpp

:: invoke it
mydiff.exe %FIRST% %SECOND% %OUT%

