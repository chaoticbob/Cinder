#include "cinder/app/App.h"
#include "cinder/app/RendererVk.h"
#include "cinder/vk/vk.h"
#include "cinder/GeomIo.h"
#include "cinder/ImageIo.h"
using namespace ci;
using namespace ci::app;
using namespace std;

/** \class BasicApp
 *
 */
class Float32FramebuffersApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;

private:
	vk::TextureRef		mTex;

	vk::TextureRef		mAttachmentTex0;
	vk::TextureRef		mAttachmentTex1;
	vk::RenderPassRef	mRenderPass;
	vk::FramebufferRef	mFramebuffer;
	vk::ShaderProgRef	mShader;
	
	void drawBlendingTests();
};

void Float32FramebuffersApp::setup()
{
	Surface surf8u = Surface( loadImage( getAssetPath( "bloom.jpg" ) ) );
	mTex = vk::Texture::create( surf8u );

	VkFormat textureInternalFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

	// Attachment
	vk::Texture2d::Format texFormat;
	texFormat.setInternalFormat( textureInternalFormat );
	texFormat.setUsageColorAttachment();
	texFormat.setMagFilter( VK_FILTER_NEAREST );
	texFormat.setMinFilter( VK_FILTER_NEAREST );
	texFormat.setWrap( VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
	texFormat.setCompareMode( VK_COMPARE_OP_LESS_OR_EQUAL );
	mAttachmentTex0 = vk::Texture2d::create( getWindowWidth(), getWindowHeight(), texFormat );
	mAttachmentTex1 = vk::Texture2d::create( getWindowWidth(), getWindowHeight(), texFormat );

	// Render pass
	ci::vk::RenderPass::Attachment attachment0 = ci::vk::RenderPass::Attachment( textureInternalFormat )
		.setInitialLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		.setFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	ci::vk::RenderPass::Attachment attachment1 = ci::vk::RenderPass::Attachment( textureInternalFormat )
		.setInitialLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		.setFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	auto renderPassOptions = ci::vk::RenderPass::Options()
		.addAttachment( attachment0 )
		.addAttachment( attachment1 )
		.addSubPass( ci::vk::RenderPass::Subpass().addColorAttachment( 0 ) )
		.addSubPass( ci::vk::RenderPass::Subpass().addColorAttachment( 1 ).addPreserveAttachment( 0 ) );
		renderPassOptions.addSubpassSelfDependency( 0 );
		renderPassOptions.addSubpassSelfDependency( 1 );
	mRenderPass = vk::RenderPass::create( renderPassOptions );
	
	// Framebuffer
	vk::Framebuffer::Format framebufferFormat = vk::Framebuffer::Format()
		.addAttachment( vk::Framebuffer::Attachment( mAttachmentTex0->getImageView() ) )
		.addAttachment( vk::Framebuffer::Attachment( mAttachmentTex1->getImageView() ) );
	mFramebuffer = vk::Framebuffer::create( mRenderPass->getRenderPass(), mAttachmentTex0->getSize(), framebufferFormat );

	mShader = vk::ShaderProg::create( vk::ShaderProg::Format().vertex( loadAsset( "shader.vert" ) ).fragment( loadAsset( "shader.frag" ) ) );
}

void Float32FramebuffersApp::update()
{
	mRenderPass->beginRender( vk::context()->getDefaultCommandBuffer(), mFramebuffer );
		
	vk::setMatricesWindow( getWindowSize() );

	{
		vk::draw( mTex, getWindowBounds() );
	}

	mRenderPass->nextSubpass();

	{
		vk::ScopedShaderProg shader( mShader );
		mShader->uniform( "ciBlock1.color", vec4( 0, 0, 0.5f, 0 ) );
		vk::draw( mAttachmentTex0, getWindowBounds() );
	}

	mRenderPass->endRender();
}

void Float32FramebuffersApp::draw()
{
	vk::setMatricesWindow( getWindowSize() );

	vk::ScopedShaderProg shader( mShader );
	mShader->uniform( "ciBlock1.color", vec4( 0, 0.5f, 0, 0 ) );
	vk::draw( mAttachmentTex1, getWindowBounds() );
}

CINDER_APP( Float32FramebuffersApp, RendererVk )