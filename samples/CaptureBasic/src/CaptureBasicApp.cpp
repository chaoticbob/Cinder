#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/android/CinderAndroid.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#if defined( CINDER_ANDROID )
    #define USE_HW_TEXTURE
#endif    

class CaptureBasicApp : public App {
  public:
  CaptureBasicApp();

	void setup() override;
	void update() override;
	void draw() override;

	void focusGained();
	void focusLost();

	void connectCamera();

  private:
	void printDevices();

	CaptureRef			mCapture;
	gl::TextureRef		mTexture;
};

CaptureBasicApp::CaptureBasicApp()
{
	#if defined( CINDER_ANDROID )
		ci::android::setActivityGainedFocusCallback( [this] { focusGained(); } );
		ci::android::setActivityLostFocusCallback( [this] { focusLost(); } );
	#endif
}

void CaptureBasicApp::focusGained()
{
	CI_LOG_I("Gained Focus");
	connectCamera();
}

void CaptureBasicApp::focusLost()
{
	CI_LOG_I("Lost Focus");
	if( mCapture ) {
		CI_LOG_I("Stopping Camera");
		mCapture->stop();
		mCapture.reset();
	}
}

void CaptureBasicApp::connectCamera()
{
	if( ! mCapture )
	{
		CI_LOG_I("Starting Camera");
		try {
			mCapture = Capture::create( 640, 480 );
			mCapture->start();
		}
		catch( ci::Exception &exc ) {
			CI_LOG_EXCEPTION( "Failed to init capture ", exc );
		}
  }
}

void CaptureBasicApp::setup()
{
	printDevices();
	connectCamera();
}

void CaptureBasicApp::update()
{

#if defined( USE_HW_TEXTURE )
	if( mCapture && mCapture->checkNewFrame() ) {
	    mTexture = mCapture->getTexture();
	}
#else
	if( mCapture && mCapture->checkNewFrame() ) {
		if( ! mTexture ) {
			// Capture images come back as top-down, and it's more efficient to keep them that way
			mTexture = gl::Texture::create( *mCapture->getSurface(), gl::Texture::Format().loadTopDown() );
		}
		else {
			mTexture->update( *mCapture->getSurface() );
		}
	}
#endif

}

void CaptureBasicApp::draw()
{

	gl::clear();

	if( mTexture ) {
		gl::ScopedModelMatrix modelScope;
#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
		// change iphone to landscape orientation
		gl::rotate( M_PI / 2 );
		gl::translate( 0, - getWindowWidth() );

		Rectf flippedBounds( 0, 0, getWindowHeight(), getWindowWidth() );
  #if defined( CINDER_ANDROID )
  		std::swap( flippedBounds.y1, flippedBounds.y2 );
  #endif
		gl::draw( mTexture, flippedBounds );
#else
		gl::draw( mTexture );
#endif
	}

#if DEBUG
	auto err = gl::getError();
	if( err ) {
		CI_LOG_W( "GL Error: " << gl::getErrorString( err ) );
	}
#endif
}

void CaptureBasicApp::printDevices()
{
	for( const auto &device : Capture::getDevices() ) {
		console() << "Device: " << device->getName() << " "
#if defined( CINDER_COCOA_TOUCH ) || defined( CINDER_ANDROID )
		<< ( device->isFrontFacing() ? "Front" : "Rear" ) << "-facing"
#endif
		<< endl;
	}
}

void prepareSettings( CaptureBasicApp::Settings* settings )
{
#if defined( CINDER_ANDROID )
	settings->setKeepScreenOn( true );
#endif
}

CINDER_APP( CaptureBasicApp, RendererGl, prepareSettings )
