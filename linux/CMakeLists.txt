cmake_minimum_required( VERSION 3.1 FATAL_ERROR )

set( CINDER_TOOLCHAIN_CLANG false )

if( CINDER_TOOLCHAIN_CLANG )
    set(CMAKE_TOOLCHAIN_PREFIX "llvm-" CACHE STRING "" FORCE ) 
    set( CMAKE_C_COMPILER "clang" CACHE FILEPATH "" FORCE )
    set( CMAKE_CXX_COMPILER "clang++" CACHE FILEPATH "" FORCE )
    set( CMAKE_AR "llvm-ar" CACHE FILEPATH "" FORCE )
    set( CMAKE_LINKER "llvm-link" CACHE FILEPATH "" FORCE )
    set( CMAKE_NM "llvm-nm" CACHE FILEPATH "" FORCE )
    set( CMAKE_RANLIB "llvm-ranlib" CACHE FILEPATH "" FORCE )
endif()


if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColorReset    "${Esc}[m"     )
  set(ColorBold     "${Esc}[1m"    )
  set(Red           "${Esc}[31m"   )
  set(Green         "${Esc}[32m"   )
  set(Yellow        "${Esc}[33m"   )
  set(Blue          "${Esc}[34m"   )
  set(Magenta       "${Esc}[35m"   )
  set(Cyan          "${Esc}[36m"   )
  set(White         "${Esc}[37m"   )
  set(BoldRed       "${Esc}[1;31m" )
  set(BoldGreen     "${Esc}[1;32m" )
  set(BoldYellow    "${Esc}[1;33m" )
  set(BoldBlue      "${Esc}[1;34m" )
  set(BoldMagenta   "${Esc}[1;35m" )
  set(BoldCyan      "${Esc}[1;36m" )
  set(BoldWhite     "${Esc}[1;37m" )
endif()

set( CMAKE_VERBOSE_MAKEFILE ON )

project( cinder_linux )

get_filename_component( CINDER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE )

execute_process( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE CINDER_ARCH )

set( CINDER_INC_DIR ${CINDER_DIR}/include )
set( CINDER_SRC_DIR ${CINDER_DIR}/src )
set( CINDER_LIB_DIR ${CINDER_DIR}/lib/linux/${CINDER_ARCH} )

if( "rpi2" STREQUAL "${CINDER_LINUX_TARGET}" )
	if( NOT "arm7vl" STREQUAL "${CINDER_ARCH}" )
		message( FATAL_ERROR "${BoldRed}Wrong architecture for Raspberry Pi 2: ${CINDER_ARCH}${ColorReset}" ) 
	endif()
	set( CINDER_LINUX_TARGET_RPI2 true )
endif()


set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

# Output path
set( LIBRARY_OUTPUT_PATH "${CINDER_DIR}/lib/linux/${CINDER_ARCH}" )

if( CINDER_LINUX_TARGET_RPI2 )
	message( "${BoldGreen}CINDER_ARCH        : ${CINDER_ARCH}${ColorReset}" )
	message( "${BoldGreen}CINDER_LINUX_TARGET: ${CINDER_LINUX_TARGET}${ColorReset}" )
else()
	message( "${BoldGreen}CINDER_ARCH: ${CINDER_ARCH}${ColorReset}" )
endif()

message( "" )
message( "${BoldGreen}CINDER_INC_DIR: ${CINDER_INC_DIR}${ColorReset}" )
message( "${BoldGreen}CINDER_SRC_DIR: ${CINDER_SRC_DIR}${ColorReset}" )
message( "" )
message( "${BoldGreen}LIBRARY_OUTPUT_PATH: ${LIBRARY_OUTPUT_PATH}${ColorReset}" )
message( "" )
message( "${BoldGreen}CMAKE_C_COMPILER  : ${CMAKE_C_COMPILER}${ColorReset}" )
message( "${BoldGreen}CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}${ColorReset}" )
message( "${BoldGreen}CMAKE_AR          : ${CMAKE_AR}${ColorReset}" ) 
message( "${BoldGreen}CMAKE_LINKER      : ${CMAKE_LINKER}${ColorReset}" ) 
message( "${BoldGreen}CMAKE_NM          : ${CMAKE_NM}${ColorReset}" ) 
message( "${BoldGreen}CMAKE_RANLIB      : ${CMAKE_RANLIB}${ColorReset}" ) 
message( "" )

include(CMakeModules/CinderSources.cmake)

set( C_FLAGS "-fvisibility=default -D_UNIX" )
set( CMAKE_C_FLAGS_DEBUG   "${C_FLAGS} -g" )
set( CMAKE_C_FLAGS_RELEASE "${C_FLAGS} -Os -ffast-math ${C_FLAGS}" )
  
set( CXX_FLAGS "${CXX_FLAGS} -std=c++11 -fvisibility=default -D_UNIX" )
set( CMAKE_CXX_FLAGS_DEBUG    "${CXX_FLAGS} -g -fexceptions -frtti" )
set( CMAKE_CXX_FLAGS_RELEASE  "${CXX_FLAGS} -Os -fexceptions -frtti -ffast-math" )


add_library( cinder STATIC ${CINDER_C_SRC_FILES} ${CINDER_CXX_SRC_FILES} )

set(CINDER_INCLUDE_DEPENDS)
set(CINDER_LIBS_DEPENDS)

find_package( Freetype2 REQUIRED )
list(APPEND CINDER_INCLUDE_DEPENDS ${FREETYPE2_INCLUDE_DIRS} )
list(APPEND CINDER_LIBS_DEPENDS ${FREETYPE2_LIBRARIES} )

find_package( FontConfig REQUIRED )
list(APPEND CINDER_INCLUDE_DEPENDS ${FONTCONFIG_INCLUDE_DIRS} )
list(APPEND CINDER_LIBS_DEPENDS ${FONTCONFIG_LIBRARIES} )

target_include_directories( cinder PUBLIC "$<BUILD_INTERFACE:${CINDER_INCLUDE_DIRS};${CINDER_INCLUDE_DEPENDS}>")

set_target_properties( cinder PROPERTIES LINKER_LANGUAGE CXX 
                                DEBUG_POSTFIX "_d"
                                )
