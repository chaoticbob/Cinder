cmake_minimum_required( VERSION 3.1 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( Basic )

get_filename_component( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src" ABSOLUTE )

set( SRC_FILES
  ${SRC_DIR}/BasicApp.cpp
)

if( NOT TARGET cinder )
### If we are here it means that this build is not part of the cinder build
### so do your best to find cinder...
find_package( cinder REQUIRED 
                PATHS ${PROJECT_SOURCE_DIR}/../../../linux/${CMAKE_BUILD_TYPE}
                $ENV{Cinder_DIR}/linux/${CMAKE_BUILD_TYPE}
            )
endif()

set( CMAKE_CXX_FLAGS "${CXX_FLAGS} -std=c++11")

add_executable( BasicApp ${SRC_FILES} )

set_target_properties( BasicApp PROPERTIES DEBUG_POSTFIX "_d" )

target_link_libraries( BasicApp cinder)
