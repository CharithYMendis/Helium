param([int]$clean=0,[int]$cmakeclean=0,[string]$library="",[string]$halide="")

# Set the root dir of the Halide checkout
$LIBRARIES = $library
$ROOT = "$library\$halide"

$ErrorActionPreference = "Continue"

# Requires:
 # subversion for windows
 # cmake for windows
 # 7-Zip
 # git for windows
 # Visual Studio express 2013
 # .Net framework 4.5.1
 # Microsoft Build Tools 2013
 # llvm trunk checkout via svn in ROOT\llvm
 # clang trunk checkout via svn in ROOT\llvm\tools\clang

# Add the relevant tools to the path
# $env:PATH += ";C:\Program Files (x86)\Subversion\bin"
# $env:PATH += ";C:\Program Files (x86)\CMake 2.8\bin"
$env:PATH += ";C:\Program Files (x86)\Git\bin"
# $env:PATH += ";C:\Program Files (x86)\7-Zip"
$env:PATH += ";C:\Program Files (x86)\MSBuild\12.0\bin"

# Update source
svn up $LIBRARIES\llvm\tools\clang
svn up $LIBRARIES\llvm
git pull

# Build latest llvm
cd $LIBRARIES\llvm
if (! (Test-Path build-32)) {
  mkdir build-32
}
elseif ( $clean -eq 1 ) {
  rmdir build-32 /s /q
  mkdir build-32
}


cd build-32
cmake -D LLVM_ENABLE_TERMINFO=OFF -D LLVM_TARGETS_TO_BUILD='X86;ARM;NVPTX;AArch64' -D LLVM_ENABLE_ASSERTIONS=ON -D CMAKE_BUILD_TYPE=Release -G "Visual Studio 12" ..

if ($cmakeclean -eq 1){
	cmake --build . --config Release --clean-first
}
else{
	cmake --build . --config Release
}

if ($LastExitCode) {
  echo "Build failed!"
  exit $LastExitCode
}


# Build Halide
cd $ROOT
if (! (Test-Path build_32)) {
  mkdir build_32
}
elseif ( $clean -eq 1 ) {
  rmdir build-32 /s /q
  mkdir build-32
}

cd build_32
cmake -D LLVM_BIN=$LIBRARIES\llvm\build-32\Release\bin -D LLVM_INCLUDE="$LIBRARIES\llvm\include;$LIBRARIES\llvm\build-32\include" -D LLVM_LIB=$LIBRARIES\llvm\build-32\Release\lib -D LLVM_VERSION=35 -D TARGET_ARM=ON -D TARGET_NATIVE_CLIENT=OFF -D TARGET_OPENCL=ON -D TARGET_PTX=ON -D TARGET_SPIR=ON -D TARGET_X86=ON -D WITH_TEST_CORRECTNESS=ON -D WITH_TEST_ERROR=ON -D WITH_TEST_WARNING=ON -D WITH_TEST_PERFORMANCE=ON -D HALIDE_SHARED_LIBRARY=ON -G "Visual Studio 12" ..

if ($cmakeclean -eq 1){
	cmake --build . --config Release --clean-first
}
else{
	cmake --build . --config Release
}

if ($LastExitCode) {
  echo "Build failed!"
  exit $LastExitCode
}


cd $ROOT

# $COMMIT = git show HEAD | head -n1 | cut -b8-
# $DATE = date +%Y_%m_%d

if (! (Test-Path distrib_32)) {
  mkdir x86
}
cd x86
cp ..\build_32\include\Halide.h .
cp ..\build_32\lib\Release\Halide.lib .
cp ..\build_32\bin\Release\Halide.dll .
# &7z a Halide_Windows_64_trunk_${COMMIT}_${DATE}.zip Halide.h Halide.lib Halide.dll

# Run the tests
$env:HL_JIT_TARGET = "host"

cd $ROOT\build_32\bin\Release

Get-ChildItem . -filter correctness*.exe | ForEach { 
  echo ""
  echo $_.Fullname
  &$_.Fullname
  if ($LastExitCode) {
    echo "Test failed!"
    exit $LastExitCode
  }
}

Get-ChildItem . -filter performance*.exe | ForEach { 
  echo ""
  echo $_.Fullname
  &$_.Fullname
  if ($LastExitCode) {
    echo "Test failed!"
    exit $LastExitCode
  }
}





