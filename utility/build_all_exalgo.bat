:: build the entire project
@echo off

set ARCH=%1
set MODE=%2
set HALIDE_DIRECTORY=%3

set CURRENT_DIR_MAIN=%CD%

:: dr_clients
cd %EXALGO_PARENT_FOLDER%\dr_clients\utility
call build.bat %ARCH% %MODE%

:: build the pre process tools

cd %EXALGO_PARENT_FOLDER%\preprocess\code_diff\utility
call build.bat %ARCH% %MODE%
cd %EXALGO_PARENT_FOLDER%\preprocess\filter_funcs\utility
call build.bat %ARCH% %MODE%



:: build the post process tools
cd %EXALGO_PARENT_FOLDER%\postprocess\buildex\utility
call build.bat %ARCH% %MODE%

:: build the tests
cd %EXALGO_TEST_FOLDER%\c_tests\utility
call build.bat %ARCH% %MODE%
cd %EXALGO_TEST_FOLDER%\halide_tests\utility
call build.bat %ARCH% 1 %HALIDE_DIRECTORY%
call build.bat %ARCH% 0 %HALIDE_DIRECTORY%

cd %CURRENT_DIR_MAIN%