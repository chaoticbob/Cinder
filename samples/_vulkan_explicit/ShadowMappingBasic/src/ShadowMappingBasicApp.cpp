/*
 Basic implementation of shadow mapping
 Keith Butters - 2014 - http://www.keithbutters.com


 Copyright 2016 Google Inc.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.


 Copyright (c) 2016, The Cinder Project, All rights reserved.

 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererVk.h"
#include "cinder/vk/vk.h"
#include "cinder/Camera.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const int	FBO_WIDTH = 2048;
const int	FBO_HEIGHT = 2048;

class ShadowMappingBasic : public App {
  public:
	static void prepareSettings( Settings *settings );
	void setup() override;
	void resize() override;
	void update() override;
	void draw() override;
	
	void drawScene( bool shadowMap );
	
  private:
	vk::RenderTargetRef		mFbo;

	CameraPersp				mCam;
	CameraPersp				mLightCam;
	vec3					mLightPos;
	
	vk::GlslProgRef			mGlsl;
	
	vk::BatchRef			mTeapotBatch;
	vk::BatchRef			mTeapotShadowedBatch;
	vk::BatchRef			mFloorBatch;
	vk::BatchRef			mFloorShadowedBatch;
	
	float					mTime;

	VkSemaphore				mImageAcquiredSemaphore;
	VkSemaphore				mShadowMapCompleteSemaphore;
	VkSemaphore				mRenderingCompleteSemaphore;
		
	vk::CommandBufferRef	mShadowMapCmdBuf;
	vk::CommandBufferRef	mRenderCmdBuf;

	void generateShadowMapCommandbuffer( const vk::CommandBufferRef& cmdBuf );
	void generateRenderCommandBuffer( const vk::CommandBufferRef& cmdBuf );
};

void ShadowMappingBasic::prepareSettings( Settings *settings )
{
	settings->setHighDensityDisplayEnabled();
	settings->setWindowSize( 1024, 768 );
}

void ShadowMappingBasic::setup()
{	
	VkFormat depthInternalFormat = vk::findBestDepthStencilAttachmentFormat( vk::context()->getDevice() );
	CI_LOG_I( "Shadow Map Depth Format: " << vk::toStringVkFormat( depthInternalFormat ) );

	vk::Texture2d::Format texParms = vk::Texture2d::Format();
	texParms.setMagFilter( VK_FILTER_LINEAR );
	texParms.setMinFilter( VK_FILTER_LINEAR );
	texParms.setWrap( VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
	texParms.setCompareMode( VK_COMPARE_OP_LESS_OR_EQUAL );
	mFbo = vk::RenderTarget::create( ivec2( FBO_WIDTH, FBO_HEIGHT ), vk::RenderTarget::Options( depthInternalFormat ).setDepthStencilTextureParams( texParms ) );  
	
	// Setup camera
	mCam.setPerspective( 40.0f, getWindowAspectRatio(), 0.5f, 500.0f );

	// Setup camera from the light's viewpoint
	mLightPos = vec3( 0.0f, 5.0f, 1.0f );
	mLightCam.setPerspective( 100.0f, mFbo->getAspectRatio(), 0.5f, 10.0f );
	mLightCam.lookAt( mLightPos, vec3( 0.0f ) );

	// Setup sahder
	try {
		vk::ShaderProg::Format format = vk::ShaderProg::Format()
			.vertex( loadAsset("shadow_shader.vert") )
			.fragment( loadAsset("shadow_shader.frag") );

		mGlsl = vk::GlslProg::create( format );
		mGlsl->uniform( "uShadowMap", mFbo->getDepthStencilTexture() );
	}
	catch ( Exception &exc ) {
		CI_LOG_EXCEPTION( "glsl load failed", exc );
		std::terminate();
	}

	auto teapot				= geom::Teapot().subdivisions( 8 );
	auto floor				= geom::Cube().size( 10.0f, 0.5f, 10.0f );
	mTeapotBatch			= vk::Batch::create( teapot, vk::getStockShader( vk::ShaderDef() ) );
	mFloorBatch				= vk::Batch::create( floor, vk::getStockShader( vk::ShaderDef() ) );
	mTeapotShadowedBatch	= vk::Batch::create( teapot, mGlsl );
	mFloorShadowedBatch		= vk::Batch::create( floor, mGlsl );

	// Semaphores
	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vkCreateSemaphore( vk::context()->getDevice(), &semaphoreCreateInfo, nullptr, &mImageAcquiredSemaphore );
	vkCreateSemaphore( vk::context()->getDevice(), &semaphoreCreateInfo, nullptr, &mShadowMapCompleteSemaphore );
	vkCreateSemaphore( vk::context()->getDevice(), &semaphoreCreateInfo, nullptr, &mRenderingCompleteSemaphore );

	mShadowMapCmdBuf = vk::CommandBuffer::create( vk::context()->getDefaultCommandPool()->getCommandPool() );
	mRenderCmdBuf = vk::CommandBuffer::create( vk::context()->getDefaultCommandPool()->getCommandPool() );

	vk::enableDepthRead();
	vk::enableDepthWrite();
}

void ShadowMappingBasic::resize()
{
	mCam.setAspectRatio( getWindowAspectRatio() );
}

void ShadowMappingBasic::update()
{
	// Store time so each render pass uses the same value
	mTime = getElapsedSeconds();
	mCam.lookAt( vec3( sin( mTime ) * 5.0f, sin( mTime ) * 2.5f + 2, 5.0f ), vec3( 0.0f ) );
}

void ShadowMappingBasic::drawScene( bool shadowMap )
{
	vk::pushModelMatrix();
	vk::rotate( mTime * 2.0f, 1.0f, 1.0f, 1.0f );

	if( shadowMap ) {
		mTeapotBatch->draw();
	}
	else {
		mTeapotShadowedBatch->uniform( "ciBlock1.uColor", ColorA( 0.4f, 0.6f, 0.9f ) );		
		mTeapotShadowedBatch->draw();
	}
	vk::popModelMatrix();
	
	vk::translate( 0.0f, -2.0f, 0.0f );
	if( shadowMap ) {
		mFloorBatch->draw();
	}
	else {
		mFloorShadowedBatch->uniform( "ciBlock1.uColor", ColorA( 0.7f, 0.7f, 0.7f ) );
		mFloorShadowedBatch->draw();
	}
}

void ShadowMappingBasic::generateShadowMapCommandbuffer( const vk::CommandBufferRef& cmdBuf )
{
	cmdBuf->begin();
	{
		// Render shadow map
		{
			// Set polygon offset to battle shadow acne
			vk::enablePolygonOffsetFill();
			vk::polygonOffset( 2.0f, 2.0f );

			mFbo->beginRenderExplicit( cmdBuf );

			vk::ScopedMatrices pushMatrices;
			vk::setMatrices( mLightCam );
			drawScene( true );

			mFbo->endRenderExplicit();

			// Disable polygon offset for final render
			vk::disablePolygonOffsetFill();
		}
	}
	cmdBuf->end();
}

void ShadowMappingBasic::generateRenderCommandBuffer( const vk::CommandBufferRef& cmdBuf )
{
	cmdBuf->begin();
	{
		// Render shadowed pass
		vk::context()->getPresenter()->beginRender( cmdBuf, vk::context() );
		{
			// Clear if single sample, multi sample is cleared on attachment load
			if( ! vk::context()->getPresenter()->isMultiSample() ) {
				vk::context()->clearAttachments();
			}

			vk::setMatrices( mCam );

			vec4 mvLightPos	= vk::getModelView() * vec4( mLightPos, 1.0f );
			const mat4 flipY = mat4( 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );
			mat4 shadowMatrix = flipY*mLightCam.getProjectionMatrix() * mLightCam.getViewMatrix();

			mTeapotShadowedBatch->uniform( "ciBlock0.uShadowMatrix", shadowMatrix );
			mTeapotShadowedBatch->uniform( "ciBlock1.uLightPos", mvLightPos );
			mFloorShadowedBatch->uniform( "ciBlock0.uShadowMatrix", shadowMatrix );
			mFloorShadowedBatch->uniform( "ciBlock1.uLightPos", mvLightPos );

			drawScene( false ); 

			// Uncomment for debug
			/*			
			vk::setMatricesWindow( getWindowSize() );
			vk::color( 1.0f, 1.0f, 1.0f );
			float size = 0.5f*std::min( getWindowWidth(), getWindowHeight() );
			vk::draw( mShadowMapTex, Rectf( 0, 0, size, size ) );
			*/
		}
		vk::context()->getPresenter()->endRender( vk::context() );
	}
	cmdBuf->end();
}

void ShadowMappingBasic::draw()
{
	// Get next image
	vk::context()->getPresenter()->acquireNextImage( VK_NULL_HANDLE, mImageAcquiredSemaphore );

	// Build shadow map command buffer
	generateShadowMapCommandbuffer( mShadowMapCmdBuf );

	// Build render command buffer
	generateRenderCommandBuffer( mRenderCmdBuf );

    // Submit command buffers for processing
	{
		const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		vk::context()->getGraphicsQueue()->submit( mShadowMapCmdBuf, mImageAcquiredSemaphore, waitDstStageMask, VK_NULL_HANDLE, mShadowMapCompleteSemaphore );
	}

	{
		const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		vk::context()->getGraphicsQueue()->submit( mRenderCmdBuf, mShadowMapCompleteSemaphore, waitDstStageMask, VK_NULL_HANDLE, mRenderingCompleteSemaphore );
	}

	// Submit presentation
	vk::context()->getGraphicsQueue()->present( mRenderingCompleteSemaphore, vk::context()->getPresenter() );

	// Wait for work to be done
	vk::context()->getGraphicsQueue()->waitIdle();
}

VkBool32 debugReportVk(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t                   object,
    size_t                     location,
    int32_t                    messageCode,
    const char*                pLayerPrefix,
    const char*                pMessage,
    void*                      pUserData
)
{
	if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		//CI_LOG_I( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		//CI_LOG_W( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		//CI_LOG_I( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		CI_LOG_E( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		//CI_LOG_D( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	return VK_FALSE;
}

const std::vector<std::string> gLayers = {
	//"VK_LAYER_LUNARG_api_dump",
	//"VK_LAYER_LUNARG_core_validation",
	//"VK_LAYER_LUNARG_device_limits",
	//"VK_LAYER_LUNARG_image",
	//"VK_LAYER_LUNARG_object_tracker",
	//"VK_LAYER_LUNARG_parameter_validation",
	//"VK_LAYER_LUNARG_screenshot",
	//"VK_LAYER_LUNARG_swapchain",
	//"VK_LAYER_GOOGLE_threading",
	//"VK_LAYER_GOOGLE_unique_objects",
	//"VK_LAYER_LUNARG_vktrace",
	//"VK_LAYER_LUNARG_standard_validation",
};

CINDER_APP( 
	ShadowMappingBasic, 
	RendererVk( RendererVk::Options()
		.setSamples( VK_SAMPLE_COUNT_8_BIT )
		.setExplicitMode() 
		.setLayers( gLayers )
		.setDebugReportCallbackFn( debugReportVk ) 
	), 
	ShadowMappingBasic::prepareSettings 
)
