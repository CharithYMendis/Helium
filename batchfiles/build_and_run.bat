:: run release or debugs
:: hardcoded for genie and basic_tests
@echo off

set CURRENT_DIR=%CD%

call select_and_build %1 %2
cd ..\tests
call build %1 release
cd ..\batchfiles
call run %1 C:\Charith\clients\tests\build\bin\basic_tests.exe %3 %2

cd %CURRENT_DIR%





