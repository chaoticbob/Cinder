#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TextureMultisampleApp : public App {
public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::FboRef	mSingleSampleFbo;
	gl::FboRef	mMultiSampleFbo;
};

void TextureMultisampleApp::setup()
{
	// Single sample
	try {
		auto fboFmt = gl::Fbo::Format().colorTexture().disableDepth();
		mSingleSampleFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), fboFmt );
	}
	catch( const std::exception &e ) {
		CI_LOG_E( "Single sample failed: " << e.what() );
	}

	// Single sample
	try {
		auto fboFmt = gl::Fbo::Format().colorTexture( gl::Texture2d::Format().samples( 8 ) ).disableDepth();
		mMultiSampleFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), fboFmt );
	}
	catch( const std::exception &e ) {
		CI_LOG_E( "Multiple sample failed: " << e.what() );
	}
}

void TextureMultisampleApp::mouseDown( MouseEvent event )
{
}

void TextureMultisampleApp::update()
{
}

void TextureMultisampleApp::draw()
{
	// Single sample
	{
		gl::ScopedFramebuffer scopedFbo( mMultiSampleFbo );

		gl::clear( Color( 1, 0, 0 ) );

		gl::lineWidth( 3.0f );
		gl::drawStrokedCircle( getWindowCenter(), 200.0f, 12 );

		mMultiSampleFbo->blitTo( mSingleSampleFbo, mMultiSampleFbo->getBounds(), mSingleSampleFbo->getBounds() );
	}

	gl::clear( Color( 0, 0, 0 ) ); 


	gl::draw( mSingleSampleFbo->getColorTexture() );
}

CINDER_APP( TextureMultisampleApp, RendererGl )
