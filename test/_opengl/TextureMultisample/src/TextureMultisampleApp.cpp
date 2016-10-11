#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TextureMultisampleApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void TextureMultisampleApp::setup()
{
}

void TextureMultisampleApp::mouseDown( MouseEvent event )
{
}

void TextureMultisampleApp::update()
{
}

void TextureMultisampleApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( TextureMultisampleApp, RendererGl )
