cmake_minimum_required( VERSION 3.10 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

# Configure the Android toolchain before the project start
if( CINDER_TARGET )
	string( TOLOWER "${CINDER_TARGET}" CINDER_TARGET_LOWER )
	if( "android" STREQUAL "${CINDER_TARGET_LOWER}" )
	    include( "${CMAKE_CURRENT_SOURCE_DIR}/proj/cmake/modules/cinderAndroidToolchain.cmake" )
    endif()
endif()

# Project start
project( cinder )

# Enable folders for organization in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option( CINDER_DX12 "Enable DirectX 12 support" OFF )

option( CINDER_BUILD_TESTS "Build unit tests." OFF )
option( CINDER_BUILD_ALL_SAMPLES "Build all samples." OFF )
set( CINDER_BUILD_SAMPLE "" CACHE STRING "Build a specific sample by specifying its path relative to the samples directory (ex. '_opengl/Cube')." )

set( CINDER_PATH      "${CMAKE_CURRENT_SOURCE_DIR}" )
set( CINDER_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proj/cmake" )


if( MSVC )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

if( CINDER_DX12 )
	set( SLANG_ENABLE_CUDA     OFF CACHE BOOL "" FORCE )
	set( SLANG_ENABLE_OPTIX    OFF CACHE BOOL "" FORCE )
	set( SLANG_ENABLE_NVAPI    OFF CACHE BOOL "" FORCE )
	
	set( SLANG_ENABLE_SLANGD   OFF CACHE BOOL "" FORCE )
	set( SLANG_ENABLE_SLANGC   OFF CACHE BOOL "" FORCE )
	set( SLANG_ENABLE_GFX      OFF CACHE BOOL "" FORCE )
	set( SLANG_ENABLE_TESTS    OFF CACHE BOOL "" FORCE )
	set( SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE )
	set( SLANG_ENABLE_REPLAYER OFF CACHE BOOL "" FORCE )
    
    set( CMAKE_FOLDER "slang" )
	add_subdirectory( ${CINDER_PATH}/deps/slang )
    unset( CMAKE_FOLDER )
endif()

include( ${CINDER_CMAKE_DIR}/configure.cmake )
include( ${CINDER_CMAKE_DIR}/libcinder_configure.cmake )
include( ${CINDER_CMAKE_DIR}/libcinder_source_files.cmake )

set( CINDER_SKIP_SAMPLES "" )

if( CINDER_LINUX )
	include( "${CINDER_CMAKE_DIR}/platform_linux.cmake" )
elseif( CINDER_MAC )
	include( "${CINDER_CMAKE_DIR}/platform_macosx.cmake" )
elseif( CINDER_ANDROID )
	include( "${CINDER_CMAKE_DIR}/platform_android.cmake" )
elseif( CINDER_MSW )
	include( "${CINDER_CMAKE_DIR}/platform_msw.cmake" )
else()
	message( FATAL_ERROR "no target defined for system: '${CMAKE_SYSTEM_NAME}.'" )
endif()

include( ${CINDER_CMAKE_DIR}/libcinder_target.cmake )

if( CINDER_BUILD_ALL_SAMPLES )
	include( ${CINDER_CMAKE_DIR}/modules/findCMakeDirs.cmake )

	set( allSamples "" )
	findCMakeDirs( allSamples "${CINDER_PATH}/samples" "${CINDER_SKIP_SAMPLES}" )

	foreach( sampleDir ${allSamples} )
		ci_log_v( "adding sample: ${sampleDir}" )
		add_subdirectory( ${sampleDir} )
	endforeach()
elseif( CINDER_BUILD_SAMPLE )
	foreach( sampleDir ${CINDER_BUILD_SAMPLE} )
		ci_log_v( "adding sample: ${sampleDir}" )
		add_subdirectory( ${CINDER_PATH}/samples/${sampleDir}/proj/cmake )
	endforeach()
endif()

if( CINDER_BUILD_TESTS )
	enable_testing()
	add_subdirectory( ${CINDER_PATH}/test/unit/proj/cmake )
	add_custom_target( check COMMAND ${CMAKE_CTEST_COMMAND} --verbose )
endif()
