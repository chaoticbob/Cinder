cmake_minimum_required( VERSION 3.1 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

#set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
project( Basic )
get_filename_component( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src" ABSOLUTE )

set( SRC_FILES
  ${SRC_DIR}/BasicApp.cpp
)

set( CMAKE_DEBUG_POSTFIX "_d" )
add_executable( BasicApp ${SRC_FILES} )
target_link_libraries( BasicApp cinder)
