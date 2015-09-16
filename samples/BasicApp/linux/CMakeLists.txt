cmake_minimum_required( VERSION 3.1 FATAL_ERROR )

set( CMAKE_VERBOSE_MAKEFILE ON )

project( Basic )

get_filename_component( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src" ABSOLUTE )

set( SRC_FILES
  ${SRC_DIR}/BasicApp.cpp
)

add_executable( BasicApp ${SRC_FILES} )

set_target_properties( BasicApp PROPERTIES DEBUG_POSTFIX "_d" )

target_link_libraries( BasicApp cinder)
