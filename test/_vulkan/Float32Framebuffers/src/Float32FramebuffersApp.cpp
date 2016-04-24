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
	vk::TextureRef		mAttachmentTex2;
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
	mAttachmentTex2 = vk::Texture2d::create( getWindowWidth(), getWindowHeight(), texFormat );
	vk::transitionToFirstUse( mAttachmentTex0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk::context() );
	vk::transitionToFirstUse( mAttachmentTex1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk::context() );
	vk::transitionToFirstUse( mAttachmentTex2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk::context() );

	// Render pass
	ci::vk::RenderPass::Attachment attachment0 = ci::vk::RenderPass::Attachment( textureInternalFormat )
		.setInitialLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		.setFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	ci::vk::RenderPass::Attachment attachment1 = ci::vk::RenderPass::Attachment( textureInternalFormat )
		.setInitialLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		.setFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	ci::vk::RenderPass::Attachment attachment2 = ci::vk::RenderPass::Attachment( textureInternalFormat )
		.setInitialLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		.setFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	auto renderPassOptions = ci::vk::RenderPass::Options()
		.addAttachment( attachment0 )
		.addAttachment( attachment1 )
		.addAttachment( attachment2 )
		.addSubPass( ci::vk::RenderPass::Subpass().addColorAttachment( 0 ) )
		.addSubPass( ci::vk::RenderPass::Subpass().addColorAttachment( 1 ).addPreserveAttachment( 0 ) )
		.addSubPass( ci::vk::RenderPass::Subpass().addColorAttachment( 2 ).addPreserveAttachment( 1 ) );
		renderPassOptions.addSubpassSelfDependency( 0 );
		renderPassOptions.addSubpassSelfDependency( 1 );
		renderPassOptions.addSubpassSelfDependency( 2 );
	mRenderPass = vk::RenderPass::create( renderPassOptions );
	
	// Framebuffer
	vk::Framebuffer::Format framebufferFormat = vk::Framebuffer::Format()
		.addAttachment( vk::Framebuffer::Attachment( mAttachmentTex0->getImageView() ) )
		.addAttachment( vk::Framebuffer::Attachment( mAttachmentTex1->getImageView() ) )
		.addAttachment( vk::Framebuffer::Attachment( mAttachmentTex2->getImageView() ) );
	mFramebuffer = vk::Framebuffer::create( mRenderPass->getRenderPass(), mAttachmentTex0->getSize(), framebufferFormat );

	mShader = vk::ShaderProg::create( vk::ShaderProg::Format().vertex( loadAsset( "shader.vert" ) ).fragment( loadAsset( "shader.frag" ) ) );
}

void Float32FramebuffersApp::update()
{
	auto cmdBuf = vk::context()->getDefaultCommandBuffer();
	mRenderPass->beginRender( cmdBuf, mFramebuffer );
		
	vk::setMatricesWindow( getWindowSize() );

	{
		vk::draw( mTex, getWindowBounds() );

		vk::color( 1.0f, 0.0f, 0.0f );
		vk::drawSolidRect( Rectf( 0, 0, 150, 150 ) + vec2( 50, 50 ) );
	}

	mRenderPass->nextSubpass();

	{
		vk::ImageMemoryBarrierParams barrier( mAttachmentTex0->getImageView()->getImage() );
		barrier.setOldLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
		barrier.setNewLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		barrier.setSrcAccessMask( VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT );
		barrier.setDstAccessMask( VK_ACCESS_SHADER_READ_BIT );
		cmdBuf->pipelineBarrierImageMemory( barrier );

		{
			vk::ScopedShaderProg shader( mShader );
			mShader->uniform( "ciBlock1.color", vec4( 0.25f, 0, 0, 0 ) );
			vk::draw( mAttachmentTex0, getWindowBounds() );
		}

		vk::color( 0.0f, 1.0f, 0.0f );
		vk::drawSolidRect( Rectf( 0, 0, 150, 150 ) + vec2( 200, 150 ) );
	}

	mRenderPass->nextSubpass();

	{
		vk::ImageMemoryBarrierParams barrier( mAttachmentTex1->getImageView()->getImage() );
		barrier.setOldLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
		barrier.setNewLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		barrier.setSrcAccessMask( VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT );
		barrier.setDstAccessMask( VK_ACCESS_SHADER_READ_BIT );
		cmdBuf->pipelineBarrierImageMemory( barrier );

		{
			vk::ScopedShaderProg shader( mShader );
			mShader->uniform( "ciBlock1.color", vec4( 0, 0.25f, 0, 0 ) );
			vk::draw( mAttachmentTex1, getWindowBounds() );
		}

		vk::color( 0.0f, 0.0f, 1.0f );
		vk::drawSolidRect( Rectf( 0, 0, 150, 150 ) + vec2( 350, 250 ) );
	}

	mRenderPass->endRender();
}

void Float32FramebuffersApp::draw()
{
	auto cmdBuf = vk::context()->getDefaultCommandBuffer();

	vk::ImageMemoryBarrierParams barrier( mAttachmentTex2->getImageView()->getImage() );
	barrier.setOldLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
	barrier.setNewLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
	barrier.setSrcAccessMask( VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT );
	barrier.setDstAccessMask( VK_ACCESS_SHADER_READ_BIT );
	cmdBuf->pipelineBarrierImageMemory( barrier );

	vk::setMatricesWindow( getWindowSize() );
	vk::draw( mAttachmentTex2, getWindowBounds() );
}

CINDER_APP( Float32FramebuffersApp, RendererVk )