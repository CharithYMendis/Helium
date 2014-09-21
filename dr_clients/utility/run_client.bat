@echo off

Setlocal EnableDelayedExpansion

:: functrace - <filter file> <filter mode>
:: profile_global - <filter file> <filter mode> <output folder>
:: memtrace - <filter file> <filter mode> <output folder>
:: funcwrap - <filter file>
:: cpuid - none
:: inscount - <filter file> <filter mode>
:: instrace -  <filter file> <filter mode> <output_folder> <static_info_size> <instrace mode>
:: memdump - <filter file> <filter mode> <output folder>

if "%1"=="help" (
	echo usage {arch} {exec} {debug_on_off} {phase} {test_name} {filter_prefix} {filter_mode} 
	echo 	{arch} m32, m64
	echo 	{exec} photoshop, halide, ctests, asm
	echo 	{test_name} - should be the executable name that DR is running on
	echo 	{debug_on_off} - 1,0
	echo 	{client_names} - "as a single argument"
	echo 	{filter_file} - the filter file name
	echo 	{filter_mode} - module, bb, range, func, neg_module, app_pc, none
	echo 	{instrace_mode} - opndtrace, opcodetrace, disasmtrace, instrace
	exit /b
)


:: get the command line arguments into manageable variables
set ARCH=%1
set EXEC=%2
set TEST_NAME=%3
set DEBUG=%4
set PHASE=%~5
set FILTER_FILE=%6
set FILTER_MODE_STRING=%7
set INSTRACE_MODE_STRING=%8

set FILTER_MODE=
set INSTRACE_MODE=

call set_parameters %FILTER_MODE_STRING% %INSTRACE_MODE_STRING%

::set MD_FILTER_FILE=%EXALGO_FILTER_FOLDER%\memdump_filter.log
::set MD_APP_PC_FILE=%EXALGO_FILTER_FOLDER%\memdump_app_pc.log

set MD_FILTER_FILE=%EXALGO_FILTER_FOLDER%\filter_%TEST_NAME%.exe.log
set MD_APP_PC_FILE=%EXALGO_FILTER_FOLDER%\filter_%TEST_NAME%.exe_app_pc.log

set CURRENT_DIR=%CD%

set CLIENT_ARGUMENTS=-logdir %EXALGO_LOG_FOLDER% -debug %DEBUG% -log 1 -exec %EXEC%
for %%a in (%PHASE%) do (
	if "%%a" == "functrace" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -functrace %FILTER_FILE% %FILTER_MODE%
	)
	if "%%a" == "profile" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -profile %FILTER_FILE% %FILTER_MODE% %EXALGO_OUTPUT_FOLDER% %IN_IMAGE%
	)
	if "%%a" == "memtrace" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -memtrace %FILTER_FILE% %FILTER_MODE% %EXALGO_OUTPUT_FOLDER% %IN_IMAGE%
 	)
	if "%%a" == "funcwrap" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -funcwrap %FILTER_FILE%
	)
	if "%%a" == "cpuid" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -cpuid
	)
	if "%%a" == "inscount" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -inscount %FILTER_FILE% %FILTER_MODE%
	)
	if "%%a" == "instrace" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -instrace %FILTER_FILE% %FILTER_MODE% %EXALGO_OUTPUT_FOLDER% 300000 %INSTRACE_MODE% %IN_IMAGE%
	)
	if "%%a" == "memdump" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -memdump %MD_FILTER_FILE% %FILTER_MODE% %MD_APP_PC_FILE% %EXALGO_OUTPUT_FOLDER%
	)
	if "%%a" == "funcreplace" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -funcreplace %FILTER_FILE% %FILTER_MODE%
	)
	if "%%a" == "misc" (
		set CLIENT_ARGUMENTS=!CLIENT_ARGUMENTS! -misc %FILTER_FILE% %FILTER_MODE%
	)
	
)
echo %CLIENT_ARGUMENTS%

call run.bat %ARCH% %EXEC% "%CLIENT_ARGUMENTS%" %TEST_NAME%

cd %CURRENT_DIR%