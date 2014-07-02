cmake_minimum_required(VERSION 2.6)

# To match Makefiles and have just one build type per configured build
# dir, we collapse VS generator configs to a single choice.
# This must be done prior to the project() command and the var
# must be set in the cache.
if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  if (DEBUG OR "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
  else ()
    # Go w/ debug info (i#1392)
    set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo" CACHE STRING "" FORCE)
  endif ()
  # we want to use the _LOCATION_<config> property
  string(TOUPPER "${CMAKE_CONFIGURATION_TYPES}" upper)
  set(location_suffix "_${upper}")
else ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  set(location_suffix "")
endif ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")

project(tests)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

file(MAKE_DIRECTORY "../output")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../output")
# set(EXECUTABLE_OUTPUT_PATH "../output")


if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  # we don't support the Debug and Release subdirs
  foreach (config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper}
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper}
      "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper}
      "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  endforeach ()
endif ()

include_directories("include_image")
include_directories("include")


#image manipulation sources
add_executable(image_blur src_image/image_manipulation.cpp src_image/blur.cpp)
add_executable(image_invert src_image/image_manipulation.cpp src_image/invert.cpp)

target_link_libraries(image_blur Gdiplus.lib)

#basic tests sources
add_executable(basic_arith src_basic/arith.cpp)
add_executable(basic_loop_red src_basic/loop_red.cpp)
add_executable(basic_convoluted src_basic/convoluted.cpp)
add_executable(basic_data_parallel src_basic/data_parallel.cpp)
add_executable(basic_calls src_basic/calls.cpp)
add_executable(basic_recursive src_basic/recursive.cpp)
add_executable(basic_ifelse src_basic/ifelse.cpp)
add_executable(basic_producer_consumer src_basic/producer_consumer.cpp)
