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
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
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
    try {
        auto texFmt = gl::Texture2d::Format().samples( 2 );
        auto tex = gl::Texture2d::create( w, h, texFmt );
        
        auto fboFmt = gl::Fbo::Format().samples( 2 ).depthBuffer().attachment( GL_COLOR_ATTACHMENT0, tex );
        auto fbo = gl::Fbo::create( w, h, fboFmt );
        CI_LOG_I( __FUNCTION__ << " : " << "PASSED!" );
    }
    catch( const std::exception &e ) {
        CI_LOG_E( __FUNCTION__ << " : " << "FAILED!" << " (" << e.what() << ")" );
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
    try {
        auto tex = gl::Texture::create( w, h, gl::Texture2d::Format().internalFormat( GL_DEPTH_COMPONENT16 ) );
        
        auto fboFmt = gl::Fbo::Format()
            .attachment( GL_DEPTH_ATTACHMENT, tex )
            .stencilBuffer();
        auto fbo = gl::Fbo::create( w, h, fboFmt );
        CI_LOG_I( __FUNCTION__ << " : " << "PASSED!" );
    }
    catch( const std::exception &e ) {
        CI_LOG_E( __FUNCTION__ << " : " << "FAILED!" << " (" << e.what() << ")" );
    }
}

void FboAttachmentApp::setup()
{
    test001();
    test002();
}

void FboAttachmentApp::mouseDown( MouseEvent event )
{
}

void FboAttachmentApp::update()
{
}

void FboAttachmentApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( FboAttachmentApp, RendererGl )
