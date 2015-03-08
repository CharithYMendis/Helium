Helium
======

Helium: Lifting High-Performance Stencil Kernels from Stripped x86 Binaries to Halide DSL Code

Please install the following dependencies

*	Install VS2013 and have VC++ 2013 runtime. (MSDN allows free download and activation for @edu emails)
	Make sure you do a default installation with default paths.
*  SVN - download it [here](http://www.visualsvn.com/downloads/) and put it on to the PATH
*  Install Cygwin64 from [here](https://cygwin.com/install.html) and install Perl, but not its SVN!! Put Cygwin's bin into the path.
*  Install CMake's latest version from [here](http://www.cmake.org/download/)
*	Download the github repository for the project
*	Installing Dynamorio – run the batch file <repo>\utility\dr_build.bat {folder} {dr_folder_name}
*	Installing Halide – just use the nightly builds of Halide from [here](https://drive.google.com/folderview?id=0B3x1cdB8WoSDSy1ZMVZoYmhnaTQ&usp=sharing) (do not try to build it in windows, but if you insist use the powershell scripts in <repo>\utility\halide_<type>_build.ps -> but this is PAINFUL!!)


building the project (all batch files should be run inside their folders)

In the <repo>\utility folder,

1. run setup_folders.bat
2. run setup.bat
3. run build_all_exalgo.bat {arch} {debug/release} {halide_dir}

now everything should be built (hopefully)

### Project structure

* dr_clients - contains all the DynamoRIO instrumentation clients
* preprocess - contains all analysis for code localization
	+ code_cov - code coverage instrumentation invocation script
	+ code_diff - tool for diffing two code coverage files
	+ filter_funcs - tool for localizing the filter function by analyzing the memory traces and profiling information extracted through DynamoRIO instrumentation
	+ image_cons - constructing example images for analysis (the generated images are synthetic examples)
* postprocess - contains all analysis for expression extraction
	+ buildex - main analysis project which processes instruction traces and memory dumps to come up with halide version of the filter. This includes various other utilitties for debugging.


procedure for running the tools (obsolete)

1.	get the code_coverage -> preprocess\code_cov\code_cov.bat {arch}
2.	get the diff -> preprocess\code_diff\build32\bin\code_diff.exe {first} {second}  {output_file}
3.	run profile_memtrace dr_client ->  
a.	run dr_clients\utility\run_tests.bat help
4.	filter the function we need to track
a.	run preprocess\build32\bin\filter.exe 
5.	run instrace_memdump 
a.	run dr_clients\utility\run_tests.bat help
6.	run ins_distrace
a.	run dr_clients\utility\run_tests.bat help
7.	run buildex.exe
a.	run postprocess\buildex\build32\bin\buildex.exe
