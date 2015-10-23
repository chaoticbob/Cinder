set(CINDER_INCLUDE_DIRS)

if( CINDER_LINUX_EGL_RPI2 )
    list(APPEND CINDER_INCLUDE_DIRS 
		/opt/vc/include
		/opt/vc/include/interface/vcos/pthreads
		/opt/vc/include/interface/vmcs_host/linux
		${CINDER_INC_DIR} 
		${CINDER_INC_DIR}/freetype
		${CINDER_INC_DIR}/jsoncpp 
		${CINDER_SRC_DIR}/linebreak 
		${CINDER_SRC_DIR}/libtess2 
		${CINDER_SRC_DIR}/AntTweakBar
		${BOOST_INC_DIR}  
	)
else()
    list(APPEND CINDER_INCLUDE_DIRS
		${CINDER_INC_DIR} 
		${CINDER_INC_DIR}/freetype
		${CINDER_INC_DIR}/glfw
		${CINDER_INC_DIR}/jsoncpp 
		${CINDER_SRC_DIR}/linebreak 
		${CINDER_SRC_DIR}/libtess2 
		${CINDER_SRC_DIR}/AntTweakBar
		${BOOST_INC_DIR}  
	)
endif()