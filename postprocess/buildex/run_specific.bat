if "%1"=="1" (
	
	call run_expression.bat C:\Charith\Dropbox\Research\development\exalgo\log\basic_data_parallel.exe.instrace.4968.log C:\Charith\Dropbox\Research\development\exalgo\log\reverse_data.txt C:\Charith\Dropbox\Research\development\exalgo\log\expression_data.txt C:\Charith\Dropbox\Research\development\exalgo\log\debug_data.txt C:\Charith\Dropbox\Research\development\exalgo\log\halide_data.cpp

)

if "%1"=="2" (

	call run_expression.bat C:\Charith\Dropbox\Research\development\exalgo\log\image_blur.exe.instrace.2068.log C:\Charith\Dropbox\Research\development\exalgo\log\reverse_blur.txt C:\Charith\Dropbox\Research\development\exalgo\log\expression_blur.txt C:\Charith\Dropbox\Research\development\exalgo\log\debug_blur.txt C:\Charith\Dropbox\Research\development\exalgo\log\halide_blur.cpp 396403 5058816 400000

)
