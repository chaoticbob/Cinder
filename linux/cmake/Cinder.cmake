
include(${CMAKE_CURRENT_LIST_DIR}/DebugColours.cmake)

# Suppress compiler checks. Why? CMake seems to be inconsistent
# with how it checks compilers on different platforms. 
set( CMAKE_C_COMPILER_WORKS   1 )
set( CMAKE_CXX_COMPILER_WORKS 1 )

# Module path
set( CMAKE_MODULE_PATH ${CINDER_DIR}/linux/cmake )

# Find architecture name
execute_process( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE CINDER_ARCH )

set( CINDER_ARCH "armv7l" )
set( RPI2_ROOT				"/home/hai/code/rpi2/rpi2_root" )
set( RPI2_TOOLCHAIN_ROOT 	"/home/hai/code/rpi2/toolchain" )
set( RPI2_TOOLCHAIN_NAME 	"arm-unknown-linux-gnueabihf"  )
set( RPI2_TOOLCHAIN_PREFIX	"${RPI2_TOOLCHAIN_ROOT}/bin/${RPI2_TOOLCHAIN_NAME}" )

if( NOT CMAKE_CXX_COMPILER OR NOT CMAKE_C_COMPILER )
	find_package( CLANG )

    if( NOT CINDER_TOOLCHAIN_GCC AND CLANG_FOUND )
		set( CINDER_TOOLCHAIN_CLANG TRUE )

		set( CMAKE_C_COMPILER						"${CLANG_CLANG}"		CACHE FILEPATH "" FORCE )
		set( CMAKE_CXX_COMPILER						"${CLANG_CLANGXX}"		CACHE FILEPATH "" FORCE )
		set( CMAKE_AR          						"${CLANG_LLVM_AR}"		CACHE FILEPATH "" FORCE )
		set( CMAKE_LINKER       					"${CLANG_LLVM_LINK}"	CACHE FILEPATH "" FORCE )
		set( CMAKE_NM           					"${CLANG_LLVM_NM}"		CACHE FILEPATH "" FORCE )
		set( CMAKE_RANLIB       					"${CLANG_LLVM_RANLIB}"	CACHE FILEPATH "" FORCE )

		set( CMAKE_C_FLAGS_INIT						"-Wall -std=c99" 	CACHE STRING "" FORCE )
		set( CMAKE_C_FLAGS_DEBUG_INIT				"-g" 				CACHE STRING "" FORCE )
		set( CMAKE_C_FLAGS_MINSIZEREL_INIT			"-Os -DNDEBUG" 		CACHE STRING "" FORCE )
		set( CMAKE_C_FLAGS_RELEASE_INIT				"-O4 -DNDEBUG" 		CACHE STRING "" FORCE )
		set( CMAKE_C_FLAGS_RELWITHDEBINFO_INIT		"-O2 -g" 			CACHE STRING "" FORCE )
		set( CMAKE_C_FLAGS							"${CMAKE_C_FLAGS} -fmessage-length=0 " CACHE STRING "" FORCE )

		set( CMAKE_CXX_FLAGS_INIT					"-Wall" 		CACHE STRING "" FORCE )
		set( CMAKE_CXX_FLAGS_DEBUG_INIT				"-g" 			CACHE STRING "" FORCE )
		set( CMAKE_CXX_FLAGS_MINSIZEREL_INIT		"-Os -DNDEBUG" 	CACHE STRING "" FORCE ) 
		set( CMAKE_CXX_FLAGS_RELEASE_INIT			"-O4 -DNDEBUG" 	CACHE STRING "" FORCE )
		set( CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT	"-O2 -g" 		CACHE STRING "" FORCE )
		set( CMAKE_CXX_FLAGS						"${CMAKE_C_FLAGS} -fmessage-length=0 " CACHE STRING "" FORCE )

        #set(STDCXXLIB                          		"-stdlib=libstdc++" )
    else()
		# Keep these versionless
    	set( CMAKE_C_COMPILER						"gcc"	CACHE FILEPATH "" FORCE )
	    set( CMAKE_CXX_COMPILER						"g++" 	CACHE FILEPATH "" FORCE )
		set( CINDER_TOOLCHAIN_GCC 					true 	CACHE BOOL "" FORCE )
		set( CINDER_TOOLCHAIN_CLANG 				false 	CACHE BOOL "" FORCE )
    endif()
endif()

set( CMAKE_SYSROOT			"/home/hai/code/rpi2/rpi2_root" CACHE FILEPATH "" FORCE )
set( CMAKE_C_COMPILER		"${RPI2_TOOLCHAIN_PREFIX}-gcc"		CACHE FILEPATH "" FORCE )
set( CMAKE_CXX_COMPILER		"${RPI2_TOOLCHAIN_PREFIX}-g++"		CACHE FILEPATH "" FORCE )
set( CMAKE_AR          		"${RPI2_TOOLCHAIN_PREFIX}-ar"		CACHE FILEPATH "" FORCE )
set( CMAKE_LINKER       	"${RPI2_TOOLCHAIN_PREFIX}-ld"		CACHE FILEPATH "" FORCE )
set( CMAKE_NM           	"${RPI2_TOOLCHAIN_PREFIX}-nm"		CACHE FILEPATH "" FORCE )
set( CMAKE_RANLIB       	"${RPI2_TOOLCHAIN_PREFIX}-ranlib"	CACHE FILEPATH "" FORCE )
set( CINDER_TOOLCHAIN_GCC 					true 	CACHE BOOL "" FORCE )
set( CINDER_TOOLCHAIN_CLANG 				false 	CACHE BOOL "" FORCE )

# C++ flags - TODO: Add logic for the case when GCC5's new C++ ABI is desired.
set( CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0 ${STDCXXLIB} -std=c++11 -Wno-reorder -Wno-unused-private-field -Wno-unused-local-typedef" )
if( CINDER_LINUX_EGL_RPI2 )
	set( CXX_FLAGS "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard ${CXX_FLAGS}" )
endif()

if( CINDER_TOOLCHAIN_CLANG )
	# Disable these warnings, many of which are coming from Boost - append at end
	set( CXX_DISABLE_WARNINGS "-Wno-reorder -Wno-unused-function -Wno-unused-private-field -Wno-unused-local-typedef -Wno-tautological-compare -Wno-missing-braces" )
elseif( CINDER_TOOLCHAIN_GCC )
	execute_process( COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION )
	if( ( "${GCC_VERSION}" VERSION_GREATER 5.0 ) OR ( "${GCC_VERSION}" VERSION_EQUAL 5.0 ) )
		# Disable these warnings, many of which are coming from Boost - append at end
		set( CXX_DISABLE_WARNINGS "-Wno-deprecated-declarations" )
	endif()
endif()

# C++ flags
set( CMAKE_CXX_FLAGS_DEBUG    "${CXX_FLAGS} -g -fexceptions -frtti" 				CACHE STRING "" FORCE )
set( CMAKE_CXX_FLAGS_RELEASE  "${CXX_FLAGS} -Os -fexceptions -frtti -ffast-math" 	CACHE STRING "" FORCE )


