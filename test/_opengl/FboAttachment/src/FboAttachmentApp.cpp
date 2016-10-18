#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
using namespace ci;
using namespace ci::app;
using namespace std;

class FboAttachmentApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	bool			mUseMultisample = true;
	bool			mDrawDepth = false;
	gl::BatchRef	mBatch;
	gl::FboRef		mSingleSampleFbo;
	gl::FboRef		mMultisampleFbo;
};

// Mix multisample texture and renderbuffer
//
// Android :
// iOS     :
// Linux   :
// macOS   : Pass - macOS does will force single sample texture
// Windows : Fail
void test001()
{
    int w = getWindowWidth();
    int h = getWindowHeight();
	std::string s;

	s = "Multisample color texture and multisample depth buffer";
    try {
        auto texFmt = gl::Texture2d::Format().samples( 2 );
        auto tex = gl::Texture2d::create( w, h, texFmt );
        
        auto fboFmt = gl::Fbo::Format().samples( 2 ).depthBuffer().attachment( GL_COLOR_ATTACHMENT0, tex );
        auto fbo = gl::Fbo::create( w, h, fboFmt );
		CI_LOG_I( __FUNCTION__ << " : " << "PASSED!" << " | " << s );
    }
    catch( const std::exception &e ) {
		CI_LOG_E( __FUNCTION__ << " : " << "FAILED!" << " (" << e.what() << ")" << " | " << s );
    }

	s = "Multisample depth texture and multisample color buffer";
    try {
        auto texFmt = gl::Texture2d::Format().samples( 2 ).internalFormat( GL_DEPTH_COMPONENT );
        auto tex = gl::Texture2d::create( w, h, texFmt );
        
        auto fboFmt = gl::Fbo::Format().samples( 2 ).colorBuffer().attachment( GL_DEPTH_ATTACHMENT, tex );
        auto fbo = gl::Fbo::create( w, h, fboFmt );
		CI_LOG_I( __FUNCTION__ << " : " << "PASSED!" << " | " << s );
    }
    catch( const std::exception &e ) {
		CI_LOG_E( __FUNCTION__ << " : " << "FAILED!" << " (" << e.what() << ")" << " | " << s );
    }
}

// Attempt to use depth and stencil separately
//
// Android : Fail
// iOS     : Fail
// Linux   : Fail
// macOS   : Fail
// Windows : Fail
void test002()
{
    int w = getWindowWidth();
    int h = getWindowHeight();
	std::string s;

	s = "Separate depth and stencil textures";
    try {
        auto depthTex = gl::Texture::create( w, h, gl::Texture2d::Format().internalFormat( GL_DEPTH_COMPONENT ) );
		auto stencilTex = gl::Texture::create( w, h, gl::Texture2d::Format().internalFormat( GL_STENCIL_INDEX ) );
        
        auto fboFmt = gl::Fbo::Format()
            .attachment( GL_DEPTH_ATTACHMENT, depthTex )
            .attachment( GL_STENCIL_ATTACHMENT, stencilTex );
        auto fbo = gl::Fbo::create( w, h, fboFmt );
        CI_LOG_I( __FUNCTION__ << " : " << "PASSED!" << " | " << s );
    }
    catch( const std::exception &e ) {
        CI_LOG_E( __FUNCTION__ << " : " << "FAILED!" << " (" << e.what() << ")" << " | " << s );
    }

	s = "Depth texture and stencil buffer";
    try {
        auto tex = gl::Texture::create( w, h, gl::Texture2d::Format().internalFormat( GL_DEPTH_COMPONENT16 ) );
        
        auto fboFmt = gl::Fbo::Format()
            .attachment( GL_DEPTH_ATTACHMENT, tex )
			.stencilBuffer();
        auto fbo = gl::Fbo::create( w, h, fboFmt );
        CI_LOG_I( __FUNCTION__ << " : " << "PASSED!" << " | " << s );
    }
    catch( const std::exception &e ) {
        CI_LOG_E( __FUNCTION__ << " : " << "FAILED!" << " (" << e.what() << ")" << " | " << s );
    }

	s = "Depth buffer and stencil texture";
    try {
        auto tex = gl::Texture::create( w, h, gl::Texture2d::Format().internalFormat( GL_DEPTH_COMPONENT16 ) );
        
        auto fboFmt = gl::Fbo::Format()
            .depthTexture()
			.attachment( GL_STENCIL_ATTACHMENT, tex );
        auto fbo = gl::Fbo::create( w, h, fboFmt );
        CI_LOG_I( __FUNCTION__ << " : " << "PASSED!" << " | " << s );
    }
    catch( const std::exception &e ) {
        CI_LOG_E( __FUNCTION__ << " : " << "FAILED!" << " (" << e.what() << ")" << " | " << s );
    }
}

void FboAttachmentApp::setup()
{
    //test001();
    //test002();


	try {
		auto fboFmt = gl::Fbo::Format();
		mSingleSampleFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), fboFmt );
		CI_LOG_I( *mSingleSampleFbo );
	}
	catch( const std::exception &e ) {
		CI_LOG_E( "Single Sample FBO Error: " << e.what() );
	}


	try {
		auto fboFmt = gl::Fbo::Format().samples( 4 );
		mMultisampleFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), fboFmt );
		CI_LOG_I( *mMultisampleFbo );
	}
	catch( const std::exception &e ) {
		CI_LOG_E( "Multisample FBO Error: " << e.what() );
	}

	mBatch = gl::Batch::create( geom::Cube().colors().size( vec3( 50 ) ), gl::context()->getStockShader( gl::ShaderDef().color() ) );


	gl::enableDepthRead();
	gl::enableDepthWrite();
}

void FboAttachmentApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case 'S':
		case 's': {
			mUseMultisample = ! mUseMultisample;
		}
		break;

		case 'D':
		case 'd': {
			mDrawDepth = ! mDrawDepth;
		}
		break;
	}
}

void FboAttachmentApp::mouseDown( MouseEvent event )
{
}

void FboAttachmentApp::update()
{
}

void FboAttachmentApp::draw()
{
	{
		gl::ScopedFramebuffer scopedFbo( mUseMultisample ? mMultisampleFbo : mSingleSampleFbo );

		gl::setMatricesWindowPersp( getWindowSize(), 60.0f, 300.0f, 10000.0f );
		gl::clear( Color( 0, 0, 0 ) );


		int nx = 10;
		int ny = 10;
		int nz = 10;
		for( int iz = 0; iz < nz; ++iz ) {
			for( int iy = 0; iy < ny; ++iy ) {
				for( int ix = 0; ix < nx; ++ix ) {
					float x = 150.0f * ( ix - ( nx / 2 ) ) + getWindowWidth() / 2.0f;
					float y = 150.0f * ( iy - ( ny / 2 ) ) + getWindowHeight() / 2.0f;
					float z = -( 150.0f * iz );

					gl::ScopedModelMatrix scopedModel;
					gl::translate( vec3( x, y, z ) );

					float t = getElapsedSeconds();
					float offset = iz + iy + ix;
					gl::rotate( 0.25f * t + offset, 1, 0, 0 );
					gl::rotate( 0.15f * t + offset, 0, 1, 0 );
					gl::rotate( 0.35f * t + offset, 0, 1, 1 );

					mBatch->draw();
				}
			}
		}
	}

	gl::setMatricesWindow( getWindowSize() );
	gl::clear( Color( 0, 0, 0 ) ); 

	gl::color( Color::white() );
	if( mDrawDepth ) {
		gl::draw( mUseMultisample ? mMultisampleFbo->getDepthTexture() : mSingleSampleFbo->getDepthTexture() );
	}
	else {
		gl::draw( mUseMultisample ? mMultisampleFbo->getColorTexture() : mSingleSampleFbo->getColorTexture() );
	}
}

CINDER_APP( FboAttachmentApp, RendererGl( RendererGl::Options().version( 4, 4 ) ) )