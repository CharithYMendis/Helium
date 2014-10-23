:: sets out the filtering and instrace modes
:: each batch file that uses this should have the FILTER_MODE variable defined

:: filter modes
:: 1.Filter based on the module names
:: 2.Filter based on specified bbs in the given modules
:: 3.Filter based on a range of addresses in the given modules
:: 4.Filter based on functions
:: 5.Filter based on the not being in the listed modules (opposite to (1))
:: 6.Filter based on the app_pc in the given modules
:: 7.No filtering

:: instrace modes
:: 1.operand trace
:: 2.opcode trace
:: 3.disassembly trace
:: 4.instruction trace


:: first get the filter mode from the strings
:: module, bb, range, func, neg_module, app_pc, none

if "%1" == "module" (
	set FILTER_MODE=2
)
if "%1" == "bb" (
	set FILTER_MODE=1
)
if "%1" == "range" (
	set FILTER_MODE=3
)
if "%1" == "func" (
	set FILTER_MODE=4
)
if "%1" == "neg_module" (
	set FILTER_MODE=5
)
if "%1" == "app_pc" (
	set FILTER_MODE=7
)
if "%1" == "none" (
	set FILTER_MODE=6
)
if "%1" == "nudge" (
	set FILTER_MODE=7
)

:: set instrace modes
:: opndtrace, opcodetrace, disasmtrace, instrace

if "%2" == "opndtrace" (
	SET INSTRACE_MODE=1
)
if "%2" == "opcodetrace" (
	SET INSTRACE_MODE=2
)
if "%2" == "disasmtrace" (
	SET INSTRACE_MODE=3
)
if "%2" == "instrace" (
	SET INSTRACE_MODE=4
)
if "%2" == "ins_distrace" (
	SET INSTRACE_MODE=5
)
