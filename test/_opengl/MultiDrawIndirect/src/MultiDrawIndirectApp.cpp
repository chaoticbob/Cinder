#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/gl.h"
#include "cinder/GeomIo.h"
#include "cinder/ImageIo.h"
#include "cinder/CameraUi.h"


using namespace ci;
using namespace ci::app;

using std::vector;
const int NUM_DRAWS = 1000;
struct DrawArraysIndirectCommand
{
    GLuint  vertexCount;
    GLuint  instanceCount;
    GLuint  firstVertex;
    GLuint  baseInstance;
};

class MultiDrawIndirectApp : public App {
 public:
	void	setup() override;
	void	update() override;
	void	draw() override;

	void	keyDown( KeyEvent event ) override;

  private:
    gl::VboRef          mIndirectDrawBuffer;
    gl::GlslProgRef     mGlsl;
    gl::BatchRef        mTriangles;
    gl::VboRef          mModelMatricesVbo;
    std::vector<mat4>   mModelMatricesCpu;

	CameraPersp		    mCam;
	CameraUi		    mCamUi;

};

void MultiDrawIndirectApp::setup()
{
    mModelMatricesCpu.resize( NUM_DRAWS );
    mModelMatricesVbo = gl::Vbo::create( GL_ARRAY_BUFFER,
                                        mModelMatricesCpu.size() * sizeof(mat4),
                                        nullptr,
                                        GL_DYNAMIC_DRAW );
    geom::BufferLayout layout;
    layout.append( geom::CUSTOM_0, 16, sizeof(mat4), 0, 1 );

    mIndirectDrawBuffer =  gl::Vbo::create(GL_DRAW_INDIRECT_BUFFER);
    mIndirectDrawBuffer->bufferData(  NUM_DRAWS * sizeof(DrawArraysIndirectCommand), NULL, GL_STATIC_DRAW );
    DrawArraysIndirectCommand * cmd = (DrawArraysIndirectCommand*)mIndirectDrawBuffer->mapBufferRange( 0, NUM_DRAWS * sizeof(DrawArraysIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
    for ( auto i = 0; i < NUM_DRAWS; i++)
    {
        cmd[i].vertexCount = 3;
        cmd[i].instanceCount = 1;
        cmd[i].firstVertex = i*3;
        cmd[i].baseInstance = i;
    }
    mIndirectDrawBuffer->unmap();

    vector<gl::VboMesh::Layout> bufferLayout = {
        gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::POSITION, 3 ),
    };
    
    std::vector<vec3> vertices;
    for ( auto i = 0; i < NUM_DRAWS; ++i ) {
        vertices.push_back( vec3(-1.f, -1.f, 0.f) );
        vertices.push_back( vec3(1.f, -1.f, 0.f) );
        vertices.push_back( vec3(0.f, 1.f, 0.f) );
    }
    
    auto trianglesVbo = gl::VboMesh::create( vertices.size(), GL_TRIANGLES, bufferLayout );
    trianglesVbo->bufferAttrib( geom::Attrib::POSITION, vertices );
    
    mGlsl = gl::GlslProg::create( gl::GlslProg::Format()
                                 .vertex( loadAsset( "InstancedModelMatrix.vert" ) )
                                 .fragment( loadAsset( "InstancedModelMatrix.frag" ) ) );

    trianglesVbo->appendVbo( layout, mModelMatricesVbo );   
    
    mTriangles = gl::Batch::create( trianglesVbo, mGlsl,{ { geom::CUSTOM_0, "aModelMatrix" } }  );

    mCam.setPerspective( 90.0f, getWindowAspectRatio(), .01f, 1000.0f );
    mCam.lookAt( vec3( 0, 0, 10 ), vec3( 0 ) );
    mCamUi.setCamera( &mCam );

}

void MultiDrawIndirectApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'w' )
		gl::setWireframeEnabled( ! gl::isWireframeEnabled() );
}

void MultiDrawIndirectApp::update()
{
    int i = 0;
    float seperation = 8.0f;
    for( auto & mat : mModelMatricesCpu ) {
        float zOffset   = ( ( ( float(i) / NUM_DRAWS ) - 0.5f ) * seperation * 1.0) + ( seperation * .3 );
        float timeVal   = getElapsedSeconds() + i;
        float sinVal    = sin( timeVal )*4;
        float cosVal    = cos( timeVal )*3;
        
        auto position   = vec3( cosVal, sinVal, zOffset );
        auto rotation   = quat( vec3( 0, timeVal, 0 ) );
        auto scale      = vec3( ( sinVal + 1.0f ) / 3.0f + .7f );
        
        mat = mat4();
        mat *= ci::translate( position );
        mat *= ci::toMat4( rotation );
        mat *= ci::scale( scale );
        i++;
    }
    
    mModelMatricesVbo->bufferSubData( 0, mModelMatricesCpu.size() * sizeof( mat4 ), mModelMatricesCpu.data() );
}

void MultiDrawIndirectApp::draw()
{
	gl::clear( Color( 0.15f, 0.15f, 0.15f ) );
    gl::color( 1.0, 1.0, .7 );
	gl::setMatrices( mCam );
    gl::ScopedGlslProg ScopedGlslProg( mTriangles->getGlslProg() );  
    auto vao = mTriangles->getVao();
    gl::ScopedVao ScopedVao( vao );
    gl::setDefaultShaderVars();
    glMultiDrawArraysIndirect( GL_TRIANGLES, NULL, NUM_DRAWS, 0 );

}


CINDER_APP( MultiDrawIndirectApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
