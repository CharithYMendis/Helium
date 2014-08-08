:: sets up the environment variables needed for building and file creation

@echo off
set CURRENT_DIR=%CD%
cd ..

:: setup the directories permanently; this script will have to be run only once
setx EXALGO_PARENT_FOLDER %CD%
setx EXALGO_HALIDE_FOLDER %CD%\generated_files\halide_files
setx EXALGO_OUTPUT_FOLDER %CD%\generated_files\output_files
setx EXALGO_FILTER_FOLDER %CD%\generated_files\filter_files
setx EXALGO_LOG_FOLDER	%CD%\log
setx EXALGO_IMAGE_FOLDER %CD%\images
setx EXALGO_TEST_FOLDER	%CD%\tests

cd %CURRENT_DIR%