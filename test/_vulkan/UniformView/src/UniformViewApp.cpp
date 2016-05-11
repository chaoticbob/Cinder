#include "cinder/app/App.h"
#include "cinder/app/RendererVk.h"
#include "cinder/vk/vk.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;

class UniformViewAppApp : public App {
  public:	
	void	setup() override;
	void	resize() override;
	void	update() override;
	void	draw() override;
	
	CameraPersp										mCam;
	vk::BatchRef									mBatch;
	vk::ShaderProgRef								mShader;
	std::vector<vk::UniformView::BufferGroupRef>	mUniformBuffers;

	struct Transform {
		vec3	pos = vec3( 0 );
		float	rot = 0;
		float	inc = 0;
	};
	std::vector<Transform>							mTransforms;
};

void UniformViewAppApp::setup()
{
	mCam.lookAt( vec3( 0, 0, 4 ), vec3( 0 ) );

	mShader = vk::getStockShader( vk::ShaderDef().color() );
	mBatch = vk::Batch::create( geom::Cube(), mShader );

	size_t n = 100;
	size_t m = 100;
	for( size_t j = 0; j < m; ++j ) {
		for( size_t i = 0; i < n; ++i ) {
			Transform xform;
			xform.pos = 2.0f*vec3 ( i - n/2.0f, j - m/2.0f, -35 ) + vec3( 1, 1, 0 );
			xform.rot = 0.0f;
			xform.inc = ci::randFloat( 1.0f, 4.0f );
			mTransforms.push_back( xform );
		}
	}

	for( size_t i = 0; i < mTransforms.size(); ++i ) {
		auto buffers = mBatch->getUniformSet()->allocateBuffers();
		mUniformBuffers.push_back( buffers );
	}

	vk::enableDepthWrite();
	vk::enableDepthRead();
}

void UniformViewAppApp::resize()
{
	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
	vk::setMatrices( mCam );
}

void UniformViewAppApp::update()
{
	for( size_t i = 0; i < mTransforms.size(); ++i ) {
		auto& xform = mTransforms[i];
		xform.rot += 0.5f + 0.2f*xform.inc;
	}
}

void UniformViewAppApp::draw()
{
	mBatch->bind();
	for( size_t i = 0; i < mTransforms.size(); ++i ) {
		const auto& xform = mTransforms[i];
		mat4 mat = glm::translate( xform.pos );
		mat *= glm::rotate( toRadians( xform.rot ), vec3( 0, 1, 0 ) );

		vk::ScopedModelMatrix pushModel;
		vk::multModelMatrix( mat );
		
		auto& buffers = mUniformBuffers[i];
		mBatch->getUniformSet()->setBuffers( buffers );
		mBatch->setDefaultUniformVars( vk::context() );
		mBatch->getUniformSet()->bufferPending( vk::context()->getCommandBuffer() );
	}

	vk::context()->getCommandBuffer()->pipelineBarrierGlobalMemoryUniformTransfer();

	
	for( size_t i = 0; i < mTransforms.size(); ++i ) {
		auto& buffers = mUniformBuffers[i];
		mBatch->getUniformSet()->setBuffers( buffers );
		mBatch->draw();
	}
	
	mBatch->unbind();

	if( 0 == ( getElapsedFrames() % 60 ) ) {
		CI_LOG_I( "FPS: " << getAverageFps() );
	}
}

static void prepareSettings( App::Settings *settings )
{
	//settings->disableFrameRate();
	settings->setWindowSize( 1024, 768 );
}

CINDER_APP( 
	UniformViewAppApp, 
	RendererVk( RendererVk::Options().setSamples( VK_SAMPLE_COUNT_1_BIT ) ),
	prepareSettings
)
