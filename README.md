exalgo
======

Extracting algorithms from binaries

Please see documentation for implementation and conventions used


1.	Install VS2013 and have VC++ 2013 runtime. (MSDN allows free download and activation for @edu emails)
	Make sure you do a default installation with default paths.
2.  SVN - http://www.visualsvn.com/downloads/ download it here and put it on to the path
3.  Install Cygwin64 https://cygwin.com/install.html and install Perl, but not its SVN!! Put Cygwin's bin into the path.
4.  CMake latest version  http://www.cmake.org/download/
5.	Download the github repository for the project
6.	Installing Dynamorio – run the batch file <repo>\utility\dr_build.bat {folder} {dr_folder_name}
7.	Installing Halide – just use the nightly builds of Halide (do not try to build it in windows 
OR use the powershell scripts in <repo>\utility\halide_<type>_build.ps -> but this is PAINFUL!!
https://drive.google.com/folderview?id=0B3x1cdB8WoSDSy1ZMVZoYmhnaTQ&usp=sharing


building the project (all batch files should be run inside their folder)

run setup_folders.bat
run setup.bat
run build_all_exalgo.bat {arch} {debug/release} {halide_dir}

now everything should be built (hopefully)

procedure for running the tools

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
