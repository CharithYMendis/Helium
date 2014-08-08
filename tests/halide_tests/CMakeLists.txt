#functions for creating the output
project(halide_tests)
cmake_minimum_required(VERSION 2.8)

find_library(GDIPLUS gdiplus)

function(halide_project name library)
  
  message("ARGS\n ${ARGN}")
  
  add_executable("${name}" ${ARGN})
  target_link_libraries("${name}" ${library} ${GDIPLUS})
  if (WIN32)
    set_target_properties("${name}" PROPERTIES LINK_FLAGS "/STACK:8388608,1048576")
  else()
    set_target_properties("${name}" PROPERTIES LINK_FLAGS "-ldl -lpthread -lz")
  endif()

endfunction(halide_project)

macro(change_config)
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
endmacro(change_config)

#end of functions


set(HALIDE_OWN_BUILD OFF)

if(HALIDE_OWN_BUILD)
	set(HALIDE_DIRECTORY "C:/Charith/libraries/Halide_build")
else(HALIDE_OWN_BUILD)
	set(HALIDE_DIRECTORY "C:/Charith/libraries/Halide_prebuilt")
endif(HALIDE_OWN_BUILD)

message(STATUS "${TYPE}")

if( "${TYPE}" STREQUAL "x64" )
	set(HALIDE_DIRECTORY "${HALIDE_DIRECTORY}/x64")
elseif( "${TYPE}" STREQUAL "x86" )
	set(HALIDE_DIRECTORY "${HALIDE_DIRECTORY}/x86")
else()
	message(FATAL_ERROR "type should be either x86 or x64")
endif()

#halide filters and tests
set(HALIDE_FILTER_DIRECTORY "${PROJECT_SOURCE_DIR}/srcs_halide")
set(HALIDE_TEST_DIRECTORY "${PROJECT_SOURCE_DIR}/srcs_tests")

#debug commands
message("halide directory - ${HALIDE_DIRECTORY}")
message("project source dir - ${PROJECT_SOURCE_DIR}")

#get Halide.h and Halide.lib
find_library(HALIDE Halide
		PATHS ${HALIDE_DIRECTORY}
		NO_DEFAULT_PATH)
include_directories("${HALIDE_DIRECTORY}")


#this is specific for each build

macro(build_app name)

	#set the cmake build output directory for intermediate filters
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output_halide_${TYPE})
	change_config()
	
	#halide filter build
	#builds the target for compiling the code and then running the output / also copies the Halide libraries
	halide_project(${name} ${HALIDE} "${HALIDE_FILTER_DIRECTORY}/${name}.cpp")
	add_custom_command(TARGET ${name} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${HALIDE_DIRECTORY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	add_custom_command(TARGET ${name} POST_BUILD
				   COMMAND cd ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
				   COMMAND ${name}
                   COMMENT "Building ${name}...")
	
	# add_custom_command(OUTPUT "${CMAKE_SOURCE_DIR}/output_halide_${TYPE}/${name}_gen.o"
                   # COMMAND cd ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
				   # COMMAND ${name}
				   # DEPENDS ${name}.cpp
                   # COMMENT "Building ${name}...")
	
	#halide app build
	include_directories(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	link_directories(${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin64")
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin64")
	else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/bin32")
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin32")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	
	#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output_tests_${TYPE})
	
	change_config()

	if(FIRST)
		halide_project(${name}_test ${HALIDE} "${HALIDE_TEST_DIRECTORY}/${name}_test.cpp" ${ARGN})
	else()
		halide_project(${name}_test ${HALIDE} "${HALIDE_TEST_DIRECTORY}/${name}_test.cpp" "${CMAKE_SOURCE_DIR}/output_halide_${TYPE}/${name}_gen.o" ${ARGN})
	endif()
	
	#add_dependencies(${name}_test ${name})
	
endmacro(build_app)



build_app(halide_blur)
build_app(halide_rotate)
build_app(halide_input_dep)



message("generation successful!")
  