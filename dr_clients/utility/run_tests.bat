@echo off

if "%1"=="help" (
	echo usage {arch} {exec} {debug_on_off} {phase} {test_name} {filter_prefix} {filter_mode} 
	echo 	{arch} m32, m64
	echo 	{exec} photoshop, halide, ctests, asm
	echo 	{test_name} - should be the executable name that DR is running on
	echo 	{debug_on_off} - 1,0
	echo 	{phase} - phase of running dr clients
	echo 	{filter_file} - the filter file name
	echo 	{filter_mode} - module, bb, range, func, neg_module, app_pc, none
	exit /b
)

set ARCH=%1
set EXEC=%2
set TEST_NAME=%3
set DEBUG=%4
set DR_PHASE=%5
set FILTER_FILE=%6
set FILTER_MODE_STRING=%7


if "%DR_PHASE%" == "profile_memtrace" (
	del %EXALGO_OUTPUT_FOLDER%\memtrace_%TEST_NAME%*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "functrace profile memtrace" %FILTER_FILE% %FILTER_MODE_STRING% %INSTRACE_MODE_STRING%	
)

if "%DR_PHASE%" == "profile" (
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "functrace profile" %FILTER_FILE% %FILTER_MODE_STRING% %INSTRACE_MODE_STRING%
)

if "%DR_PHASE%" == "opndtrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%_1*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% opndtrace
)

if "%DR_PHASE%" == "opcodetrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%_2*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% opcodetrace
)

if "%DR_PHASE%" == "disasmtrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%_3*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% disasmtrace
)

if "%DR_PHASE%" == "instrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%_4*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% instrace
)


