#include "cinder/ObjLoader.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererVk.h"
#include "cinder/vk/vk.h"
#include "cinder/Arcball.h"
#include "cinder/CameraUi.h"
#include "cinder/Sphere.h"
#include "cinder/ImageIo.h"
#include "cinder/ip/Checkerboard.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;

class ObjLoaderApp : public App {
  public:
	void	setup() override;

	void	mouseDown( MouseEvent event ) override;
	void	mouseDrag( MouseEvent event ) override;
	void	keyDown( KeyEvent event ) override;

	void	loadObj( const DataSourceRef &dataSource );
	void	writeObj();
	void	frameCurrentObject();
	void	draw() override;
	
	Arcball			mArcball;
	CameraUi		mCamUi;
	CameraPersp		mCam;
	TriMeshRef		mMesh;
	Sphere			mBoundingSphere;
	vk::BatchRef	mBatch;
	vk::GlslProgRef	mGlsl;
	vk::TextureRef	mCheckerTexture;
};

void ObjLoaderApp::setup()
{
	mCam.setPerspective( 45.0f, getWindowAspectRatio(), 0.1, 10000 );
	mCamUi = CameraUi( &mCam );

	mCheckerTexture = vk::Texture::create( ip::checkerboard( 512, 512, 32 ) );

	try {
		mGlsl = vk::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
		mGlsl->uniform( "uTex0", mCheckerTexture );
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Shader Error: " << e.what() );
	}

	loadObj( loadAsset( "8lbs.obj" ) );

	mArcball = Arcball( &mCam, mBoundingSphere );

	vk::context()->getPresenter()->setClearColor( Color( 0.0f, 0.1f, 0.2f ) );
}

void ObjLoaderApp::mouseDown( MouseEvent event )
{
	if( event.isMetaDown() )
		mCamUi.mouseDown( event );
	else
		mArcball.mouseDown( event );
}

void ObjLoaderApp::mouseDrag( MouseEvent event )
{
	if( event.isMetaDown() )
		mCamUi.mouseDrag( event );
	else
		mArcball.mouseDrag( event );
}

void ObjLoaderApp::loadObj( const DataSourceRef &dataSource )
{
	ObjLoader loader( dataSource );
	mMesh = TriMesh::create( loader );

	if( ! loader.getAvailableAttribs().count( geom::NORMAL ) )
		mMesh->recalculateNormals();

	mBatch = vk::Batch::create( *mMesh, mGlsl );
	
	mBoundingSphere = Sphere::calculateBoundingSphere( mMesh->getPositions<3>(), mMesh->getNumVertices() );
	mArcball.setSphere( mBoundingSphere );
}

void ObjLoaderApp::writeObj()
{
	fs::path filePath = getSaveFilePath();
	if( ! filePath.empty() ) {
		CI_LOG_I( "writing mesh to file path: " << filePath );
		ci::writeObj( writeFile( filePath ), mMesh );
	}
}

void ObjLoaderApp::frameCurrentObject()
{
	mCam = mCam.calcFraming( mBoundingSphere );
}

void ObjLoaderApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'o' ) {
		fs::path path = getOpenFilePath();
		if( ! path.empty() ) {
			loadObj( loadFile( path ) );
		}
	}
	else if( event.getChar() == 'f' ) {
		frameCurrentObject();
	}
	else if( event.getChar() == 'w' ) {
		writeObj();
	}
}

void ObjLoaderApp::draw()
{
	vk::enableDepthWrite();
	vk::enableDepthRead();
	
	vk::setMatrices( mCam );

	vk::pushMatrices();
		vk::rotate( mArcball.getQuat() );
		mBatch->draw();
	vk::popMatrices();
}

VkBool32 debugReportVk(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t                   object,
    size_t                     location,
    int32_t                    messageCode,
    const char*                pLayerPrefix,
    const char*                pMessage,
    void*                      pUserData
)
{
	if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		//CI_LOG_I( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		//CI_LOG_W( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		//CI_LOG_I( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		CI_LOG_E( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		//CI_LOG_D( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	return VK_FALSE;
}

const std::vector<std::string> gLayers = {
	//"VK_LAYER_LUNARG_api_dump",
	//"VK_LAYER_LUNARG_core_validation",
	//"VK_LAYER_LUNARG_device_limits",
	//"VK_LAYER_LUNARG_image",
	//"VK_LAYER_LUNARG_object_tracker",
	//"VK_LAYER_LUNARG_parameter_validation",
	//"VK_LAYER_LUNARG_screenshot",
	//"VK_LAYER_LUNARG_swapchain",
	//"VK_LAYER_GOOGLE_threading",
	//"VK_LAYER_GOOGLE_unique_objects",
	//"VK_LAYER_LUNARG_vktrace",
	//"VK_LAYER_LUNARG_standard_validation",
};

CINDER_APP( 
	ObjLoaderApp, 
	RendererVk( RendererVk::Options()
		.setSamples( VK_SAMPLE_COUNT_8_BIT )
		.setLayers( gLayers )
		.setDebugReportCallbackFn( debugReportVk ) 
	) 
)