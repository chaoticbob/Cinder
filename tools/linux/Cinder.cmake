
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

execute_process( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE CINDER_ARCH )

set( CINDER_INC_DIR ${CINDER_DIR}/include )
set( CINDER_SRC_DIR ${CINDER_DIR}/src )

set( CINDER_LIB_DIR ${CINDER_DIR}/lib/linux/${CINDER_ARCH} )

set( CINDER_TOOLCHAIN_CLANG true )

if( CINDER_TOOLCHAIN_CLANG )
    set(CMAKE_TOOLCHAIN_PREFIX 					"llvm-"		CACHE STRING "" FORCE ) 
    set( CMAKE_C_COMPILER      "clang"                 CACHE FILEPATH "" FORCE )
    set( CMAKE_CXX_COMPILER    "clang++"               CACHE FILEPATH "" FORCE )
    set( CMAKE_AR           "llvm-ar"          CACHE FILEPATH "" FORCE )
    set( CMAKE_LINKER       "llvm-link"        CACHE FILEPATH "" FORCE )
    set( CMAKE_NM           "llvm-nm "                 CACHE FILEPATH "" FORCE )
    set( CMAKE_RANLIB       "llvm-ranlib"      CACHE FILEPATH "" FORCE )

endif()

set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ -std=c++11 -Wno-reorder -Wno-unused-private-field -Wno-unused-local-typedef" CACHE STRING "" FORCE )
set( CMAKE_CXX_FLAGS_DEBUG    "${CXX_FLAGS} -g -fexceptions -frtti" 				CACHE STRING "" FORCE )
set( CMAKE_CXX_FLAGS_RELEASE  "${CXX_FLAGS} -Os -fexceptions -frtti -ffast-math" 	CACHE STRING "" FORCE )
