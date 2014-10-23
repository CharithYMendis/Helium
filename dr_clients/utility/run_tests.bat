@echo off

if "%1"=="help" (
	echo usage {arch} {exec} {test_name} {debug_on_off} {phase} {filter_file} {filter_mode} {input_image} {output_image}
	echo 	{arch} m32, m64
	echo 	{exec} photoshop, halide, ctests, asm, gimp
	echo 	{test_name} - should be the executable name that DR is running on {without exe}
	echo 	{debug_on_off} - 1,0
	echo 	{phase} - phase of running dr clients "profile memtrace" "profile" "opndtrace" "opcodetrace" "disasmtrace" "instrace" "memdump" "ins_distrace" "funcreplace" "instrace_memdump"
	echo 	{filter_file} - the filter file name {only filename with ext}
	echo 	{filter_mode} - module, bb, range, func, neg_module, app_pc, none, nudge
	echo 	{input_image} - only the filename {with ext}
	echo 	{output_image} - only the filename {with ext}
	exit /b
)


set ARCH=%1
set EXEC=%2
set TEST_NAME=%3
set DEBUG=%4
set DR_PHASE=%5
set FILTER_FILENAME=%6
set FILTER_MODE_STRING=%7
set IN_IMAGE=%8
set OUT_IMAGE=%9

set FILTER_FILE=%EXALGO_FILTER_FOLDER%\%FILTER_FILENAME%


if "%DR_PHASE%" == "profile_memtrace" (

	del %EXALGO_OUTPUT_FOLDER%\memtrace_%TEST_NAME%.exe_%IN_IMAGE%*	
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "profile memtrace" %FILTER_FILE% %FILTER_MODE_STRING% %INSTRACE_MODE_STRING%	
)


if "%DR_PHASE%" == "memtrace" (

	del %EXALGO_OUTPUT_FOLDER%\memtrace_%TEST_NAME%.exe_%IN_IMAGE%*	
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap memtrace" %FILTER_FILE% %FILTER_MODE_STRING% %INSTRACE_MODE_STRING%	
)


if "%DR_PHASE%" == "memdump" (

	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "memdump" %FILTER_FILE% %FILTER_MODE_STRING%
)

if "%DR_PHASE%" == "funcreplace" (

	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcreplace" %FILTER_FILE% %FILTER_MODE_STRING%
)

if "%DR_PHASE%" == "profile" (
	del %EXALGO_OUTPUT_FOLDER%\profile_%TEST_NAME%.exe_%IN_IMAGE%*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "profile" %FILTER_FILE% %FILTER_MODE_STRING% %INSTRACE_MODE_STRING%
)

if "%DR_PHASE%" == "opndtrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%.exe_%IN_IMAGE%_opnd*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% opndtrace
)

if "%DR_PHASE%" == "opcodetrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%.exe_%IN_IMAGE%_opcode*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% opcodetrace
)

if "%DR_PHASE%" == "disasmtrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%.exe_%IN_IMAGE%_disasm*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% disasmtrace
)

if "%DR_PHASE%" == "ins_distrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%.exe_%IN_IMAGE%_asm_instr*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% ins_distrace
)

if "%DR_PHASE%" == "funcwrap" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%.exe_%IN_IMAGE%_instr*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap" %FILTER_FILE% %FILTER_MODE_STRING% instrace
)

if "%DR_PHASE%" == "instrace" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%.exe_%IN_IMAGE%_instr*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace" %FILTER_FILE% %FILTER_MODE_STRING% instrace
)

if "%DR_PHASE%" == "memdump" (
	del %EXALGO_OUTPUT_FOLDER%\memdump*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap memdump" %FILTER_FILE% %FILTER_MODE_STRING% instrace
)


if "%DR_PHASE%" == "instrace_memdump" (
	del %EXALGO_OUTPUT_FOLDER%\instrace_%TEST_NAME%.exe_%IN_IMAGE%_instr*
	del %EXALGO_OUTPUT_FOLDER%\memdump*
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap instrace memdump" %FILTER_FILE% %FILTER_MODE_STRING% instrace
)

if "%DR_PHASE%" == "funcwrap" (
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "funcwrap" %FILTER_FILE%
)

if "%DR_PHASE%" == "misc" (
	call run_client %ARCH% %EXEC% %TEST_NAME% %DEBUG% "misc" %FILTER_FILE% %FILTER_MODE_STRING%
)





