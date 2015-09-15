cmake_minimum_required( VERSION 3.1 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

get_filename_component( CINDER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE )

include( ${CINDER_DIR}/tools/linux/Cinder.cmake )
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
project( Basic )
get_filename_component( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src" ABSOLUTE )

message( "${BoldGreen}CINDER_INC_DIR: ${CINDER_INC_DIR}${ColorReset}" )
message( "${BoldGreen}CINDER_SRC_DIR: ${CINDER_SRC_DIR}${ColorReset}" )
message( "${BoldGreen}CINDER_DIR: ${CINDER_DIR}${ColorReset}" )
message( "" )
message( "${BoldGreen}SRC_DIR       : ${SRC_DIR}${ColorReset}" )
message( "" )

include_directories( 
    ${CINDER_INC_DIR} 
    ${CINDER_INC_DIR}/glfw
    ${BOOST_INC_DIR}  
)

set( SRC_FILES
  ${SRC_DIR}/BasicApp.cpp
)

set(required_libs)
find_package( Freetype2 REQUIRED )
if( FREETYPE2_FOUND )
include_directories( ${FREETYPE2_INCLUDE_DIRS} )
list( APPEND required_libs ${FREETYPE2_LIBRARIES} )
endif( FREETYPE2_FOUND )
find_package( FontConfig REQUIRED )
if( FONTCONFIG_FOUND )
include_directories( ${FONTCONFIG_INCLUDE_DIRS} )
list( APPEND required_libs ${FONTCONFIG_LIBRARIES} )
endif( FONTCONFIG )

set( CMAKE_DEBUG_POSTFIX "_d" )
add_executable( "${CMAKE_PROJECT_NAME}" ${SRC_FILES} )
target_link_libraries( "${CMAKE_PROJECT_NAME}" debug     ${CINDER_DIR}/lib/linux/libcinder_d.a )
target_link_libraries( "${CMAKE_PROJECT_NAME}" optimized ${CINDER_DIR}/lib/linux/x86_64/libcinder.a )
target_link_libraries( "${CMAKE_PROJECT_NAME}" general libboost_filesystem.a )
target_link_libraries( "${CMAKE_PROJECT_NAME}" general libboost_system.a )
#target_link_libraries( "${CMAKE_PROJECT_NAME}" c++ dl pthread z GL X11 Xcursor Xinerama Xrandr Xi )
target_link_libraries( "${CMAKE_PROJECT_NAME}" ${required_libs} dl pthread z GL X11 Xcursor Xinerama Xrandr Xi )

