#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
using namespace ci;
using namespace ci::app;
using namespace std;

class TextureMulitSampleApp : public App {
  public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;

  private:
	gl::FboRef		mSingleSampleFbo;
	gl::FboRef		mMultiSampleFbo;
	bool			mUseMultiSample = false;

	void drawScene();
};

void TextureMulitSampleApp::setup()
{
	try {
		gl::Texture::Format texFmt = gl::Texture::Format();
		mSingleSampleFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), gl::Fbo::Format().disableDepth().colorTexture( texFmt ) );
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Single sample FBO failed: " << e.what() );
	}

	try {
		gl::Texture::Format texFmt = gl::Texture::Format().samples( 4 );
		mMultiSampleFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), gl::Fbo::Format().disableDepth().colorTexture( texFmt ) );
	}
	catch( const std::exception& e ) {
		CI_LOG_E( "Multi sample FBO failed: " << e.what() );
	}

}

void TextureMulitSampleApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case 'm':
		case 'M': {
			mUseMultiSample = ! mUseMultiSample;
		}
		break;
	}
}

void TextureMulitSampleApp::update()
{
}

void TextureMulitSampleApp::drawScene()
{
	randSeed( 0xDEADBEEF );

	for( int i = 0; i < 30; ++i ) {
		vec2 center = randVec2() * vec2( 120, 60 ) + vec2( getWindowSize() / 4 );
		float radius = randFloat( 10.0f, 60.0f );
		gl::color( randFloat( 0.7f, 1.0f ), randFloat( 0.7f, 1.0f ), randFloat( 0.7f, 1.0f ) );
		gl::lineWidth( randFloat( 1.0f, 4.0f ) );
		gl::drawStrokedCircle( center, radius, 16 );
	}

	for( int i = 0; i < 50; ++i ) {
		float w = getWindowWidth() / 2;
		float h = getWindowHeight() / 2;
		vec2 P0 = vec2( randFloat( 5.0f, w - 5.0f ), randFloat( 5.0f, h - 5.0f ) ); 
		vec2 P1 = vec2( randFloat( 5.0f, w - 5.0f ), randFloat( 5.0f, h - 5.0f ) );
		P0 += vec2( w, 0 );
		P1 += vec2( w, 0 );
		gl::color( randFloat( 0.7f, 1.0f ), randFloat( 0.7f, 1.0f ), randFloat( 0.7f, 1.0f ) );
		gl::lineWidth( randFloat( 1.0f, 4.0f ) );
		gl::drawLine( P0, P1 );
	}

	for( int i = 0; i < 30; ++i ) {
		float w = randFloat( 10.0f, 200.0f );
		float h = randFloat( 10.0f, 200.0f );

		gl::ScopedModelMatrix scopedModel;

		vec2 center = randVec2() * vec2( getWindowSize() ) * vec2( 0.5f, 0.125f ) + vec2( getWindowWidth() / 2.0f, 0.75f * getWindowHeight() );
		gl::translate( center );
		gl::rotate( toRadians( randFloat( 0.0f, 360.0f ) ) );
		gl::translate( -( 0.5f * vec2( w,h ) ) );
		gl::lineWidth( randFloat( 1.0f, 4.0f ) );
		gl::color( randFloat( 0.7f, 1.0f ), randFloat( 0.7f, 1.0f ), randFloat( 0.7f, 1.0f ) );
		gl::drawStrokedRect( Rectf( 0, 0, w, h ) );
	}	
}

void TextureMulitSampleApp::draw()
{
	if( mUseMultiSample ) {
		gl::ScopedFramebuffer scopedFbo( mMultiSampleFbo );
		gl::viewport( mMultiSampleFbo->getSize() );
		gl::clear( Color( 0.3f, 0.3f, 0.5f ) );
		drawScene();

		//mMultiSampleFbo->blitTo( mSingleSampleFbo, mMultiSampleFbo->getBounds(), mSingleSampleFbo->getBounds() );
	}
	else {
		gl::ScopedFramebuffer scopedFbo( mSingleSampleFbo );
		gl::viewport( mSingleSampleFbo->getSize() );
		gl::clear( Color( 0.3f, 0.3f, 0.5f ) );
		drawScene();
	}

	gl::clear( Color( 0, 0, 0 ) ); 

	gl::draw( mUseMultiSample ? mMultiSampleFbo->getColorTexture() : mSingleSampleFbo->getColorTexture() );
}

CINDER_APP( TextureMulitSampleApp, RendererGl )
