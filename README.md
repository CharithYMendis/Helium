Helium: Lifting High-Performance Stencil Kernels from Stripped x86 Binaries to Halide DSL Code
==============================================================================================

All the paths mentioned here are relative to the project parent folder.

Please install the following dependencies

*	Install VS2013 and have VC++ 2013 runtime. (MSDN allows free download and activation for @edu emails)
	Make sure you do a default installation with default paths.
*  SVN - download it [here](http://www.visualsvn.com/downloads/) and put it on to the PATH
*  Install Cygwin64 from [here](https://cygwin.com/install.html) and install Perl, but not its SVN!! Put Cygwin's bin into the path.
*  Install CMake's latest version from [here](http://www.cmake.org/download/)
*	Download the github repository for the project
*	Installing Dynamorio – run the batch file utility\dr_build.bat {folder} {dr_folder_name}
*	Installing Halide – just use the nightly builds of Halide from [here](https://drive.google.com/folderview?id=0B3x1cdB8WoSDSy1ZMVZoYmhnaTQ&usp=sharing) (do not try to build it in windows, but if you insist use the powershell scripts in <repo>\utility\halide_<type>_build.ps -> but this is PAINFUL!!)

### Folder Structure

* dr_clients - contains all the DynamoRIO instrumentation clients
* preprocess - contains all analysis for code localization
	+ code_cov - code coverage instrumentation invocation script
	+ code_diff - tool for diffing two code coverage files
	+ filter_funcs - tool for localizing the filter function by analyzing the memory traces and profiling information extracted through DynamoRIO instrumentation
	+ image_cons - constructing example images for analysis (the generated images are synthetic examples)
* postprocess - contains all analysis for expression extraction
	+ buildex - main analysis project which processes instruction traces and memory dumps to come up with halide version of the filter. This includes various other utilitties for debugging.

### Building the project 

All batch files should be run inside their folders.

In the utility folder,

1. Run setup_folders.bat
2. Run setup.bat
3. Run build_all_exalgo.bat {arch} {debug/release} {halide_dir}

### Running the tools

* Run utility/automation_all.py - you can run the entire tool chain or parts of it. Please look at the help of the script to figure out command line arguments.
* There are legacy batch scripts used to automate the tools. 

### Publications

The ideas of the project are published in the following paper.

* [Helium: Lifting High-Performance Stencil Kernels from Stripped x86 Binaries to Halide DSL Code](http://groups.csail.mit.edu/commit/papers/2015/mendis-pldi15-helium.pdf)
Charith Mendis, Jeffrey Bosboom, Kevin Wu, Shoaib Kamil, Jonathan Ragan-Kelley, Sylvain Paris, Qin Zhao, Saman Amarasinghe

