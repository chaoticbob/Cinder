#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FboAttachmentsApp : public App {
public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::TextureFontRef	mFont;
	gl::FboRef			mSingleSampleFbo;
	gl::FboRef			mMultiSampleFbo;
};

void FboAttachmentsApp::setup()
{
	mFont = gl::TextureFont::create( Font( "Helvetica", 64.0f ) );

	try {
		gl::Fbo::Format fmt = gl::Fbo::Format()
			.colorTexture()
			.samples( 2 )
			.stencilBuffer()
			.depthTexture( gl::Texture2d::Format().internalFormat( GL_DEPTH24_STENCIL8 ) );
		mSingleSampleFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), fmt );
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Single sample FBO error: " << e.what() );
	}
}

void FboAttachmentsApp::mouseDown( MouseEvent event )
{
}

void FboAttachmentsApp::update()
{
}

void FboAttachmentsApp::draw()
{
	{
		gl::ScopedFramebuffer scopedFbo( mSingleSampleFbo );
		gl::ScopedBlendAlpha scopedBlend;
		gl::clear( Color( 0.25f, 0, 0 ) );

		gl::color( Color::white() );
		mFont->drawString( "Single sample FBO", vec2( 10, 100 ) );

		gl::lineWidth( 4.0f );
		gl::drawStrokedCircle( getWindowCenter(), 100.0f, 16 );

		{
			gl::ScopedModelMatrix scopedMatrix;
			gl::translate( getWindowCenter() );
			gl::rotate( toRadians( 35.0f ) );
			float s = 150.0f;
			gl::drawStrokedRect( Rectf( -s, -s, s, s ) );
		}
	}

	gl::clear( Color( 0, 0, 0 ) );
	gl::color( Color::white() );
	gl::draw( mSingleSampleFbo->getColorTexture() );
}

CINDER_APP( FboAttachmentsApp, RendererGl )
