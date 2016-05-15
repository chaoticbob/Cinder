/*
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

#include "cinder/vk/RenderTarget.h"
#include "cinder/vk/CommandBuffer.h"
#include "cinder/vk/Context.h"
#include "cinder/vk/Image.h"
#include "cinder/vk/ImageView.h"
#include "cinder/vk/wrapper.h"

namespace cinder { namespace vk {

// ------------------------------------------------------------------------------------------------ 
// RenderTarget::Pass
// ------------------------------------------------------------------------------------------------
RenderTarget::Pass::Pass( VkFormat attachment )
{
	VkImageAspectFlags imageAspect = vk::determineAspectMask( attachment );
	if( imageAspect & VK_IMAGE_ASPECT_COLOR_BIT ) {
		mColorAttachments.push_back( attachment );
	}
	else if( ( imageAspect & VK_IMAGE_ASPECT_DEPTH_BIT ) || ( imageAspect & VK_IMAGE_ASPECT_STENCIL_BIT ) ) {
		mDepthStencilAttachment = attachment;
	}
}

RenderTarget::Pass::Pass( VkFormat colorAttachment, VkFormat depthStencilAttachment )
{
	mColorAttachments.push_back( colorAttachment );
	mDepthStencilAttachment = depthStencilAttachment;
}

RenderTarget::Pass::Pass( const std::vector<VkFormat>& colorAttachments, VkFormat depthStencilAttachment )
{
	mColorAttachments = colorAttachments;
	mDepthStencilAttachment = depthStencilAttachment;
}

// ------------------------------------------------------------------------------------------------ 
// RenderTarget::Options
// ------------------------------------------------------------------------------------------------ 
RenderTarget::Options::Options( const vk::RenderTarget::Pass& pass, VkSampleCountFlagBits samples )
	: mSamples( samples )
{
	mPasses.push_back( pass );
}

// ------------------------------------------------------------------------------------------------ 
// RenderTarget
// ------------------------------------------------------------------------------------------------ 
RenderTarget::RenderTarget( const ivec2& size, const RenderTarget::Options& options, vk::Device* device )
	: mSize( size ), mBounds( 0, 0, static_cast<float>( size.x ), static_cast<float>( size.y ) )
{
	initialize( options, device );
}

RenderTarget::~RenderTarget()
{
}

void RenderTarget::initialize( const RenderTarget::Options& options, vk::Device* device )
{
	mSubpassCount = static_cast<uint32_t>( options.mPasses.size() );

	vk::RenderPass::Options renderPassOptions = vk::RenderPass::Options();
	vk::Framebuffer::Format framebufferFormat = vk::Framebuffer::Format();
	
	std::vector<VkFormat> attachmentList;
	uint32_t depthStencilAttachmentIndex = UINT32_MAX;
	for( uint32_t subpassIndex = 0; subpassIndex < mSubpassCount; ++subpassIndex ) {
		const RenderTarget::Pass& pass = options.mPasses[subpassIndex];

		vk::RenderPass::Subpass subpass = vk::RenderPass::Subpass();

		// Process color attachments
		const uint32_t colorAttachmentCount = static_cast<uint32_t>( pass.mColorAttachments.size() );
		for( uint32_t subpassColorAttachmentIndex = 0; subpassColorAttachmentIndex < colorAttachmentCount; ++subpassColorAttachmentIndex ) {
			const auto& colorAttachmentInternalFormat = pass.mColorAttachments[subpassColorAttachmentIndex];

			if( options.isMultiSample() ) {
				// Add color attachment to attachment list
				attachmentList.push_back( colorAttachmentInternalFormat );

				// Add resolve attachment to attachment list
				attachmentList.push_back( colorAttachmentInternalFormat );

				// Add color attachment to render pass
				vk::RenderPass::Attachment renderPassColorAttachment = vk::RenderPass::Attachment( colorAttachmentInternalFormat, options.mSamples );
				renderPassColorAttachment.setInitialAndFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
				renderPassOptions.addAttachment( renderPassColorAttachment );

				// Add resolve attachment to render pass
				vk::RenderPass::Attachment renderPassResolveAttachment = vk::RenderPass::Attachment( colorAttachmentInternalFormat, options.mSamples );
				renderPassResolveAttachment.setInitialAndFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
				renderPassOptions.addAttachment( renderPassResolveAttachment );

				// Add attachment to subpass
				uint32_t colorAttachmentIndex = static_cast<uint32_t>( attachmentList.size() - 2 );
				uint32_t resolveAttachmentIndex = static_cast<uint32_t>( attachmentList.size() - 1 );
				subpass.addColorAttachment( colorAttachmentIndex, resolveAttachmentIndex );

				// Add color attachment to framebuffer
				vk::Texture2d::Format colorTextureParams = options.mColorTextureParams;
				colorTextureParams.setSamples( options.mSamples );
				framebufferFormat.addAttachment( vk::Framebuffer::Attachment( colorAttachmentInternalFormat, colorTextureParams ) );

				// Add resolve attachment to framebuffer
				vk::Texture2d::Format resolveTextureParams = options.mColorTextureParams;
				resolveTextureParams.setSamples( VK_SAMPLE_COUNT_1_BIT );
				framebufferFormat.addAttachment( vk::Framebuffer::Attachment( colorAttachmentInternalFormat, resolveTextureParams ) );

				// Add to lookup
				uint64_t high = ( static_cast<uint64_t>( subpassIndex ) << 32 ) & 0xFFFFFFFF00000000ULL;
				uint64_t low = ( static_cast<uint64_t>( resolveAttachmentIndex ) << 0 ) & 0xFFFFFFFF00000000ULL;
				uint64_t key = high | low;
				std::pair<uint64_t, uint32_t> keyValue = std::make_pair( key, resolveAttachmentIndex );
				mColorAttachmentMap.push_back( keyValue );
			}
			else {
				// Add color attachment to attachment list
				attachmentList.push_back( colorAttachmentInternalFormat );

				// Add color attachment to render pass
				vk::RenderPass::Attachment renderPassAttachment = vk::RenderPass::Attachment( colorAttachmentInternalFormat, options.mSamples );
				renderPassAttachment.setInitialAndFinalLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
				renderPassOptions.addAttachment( renderPassAttachment );

				// Add attachment to subpass
				uint32_t colorAttachmentIndex = static_cast<uint32_t>( attachmentList.size() - 1 );
				subpass.addColorAttachment( colorAttachmentIndex );

				// Add attachment to framebuffer
				vk::Texture2d::Format textureParams = options.mColorTextureParams;
				textureParams.setSamples( options.mSamples );
				framebufferFormat.addAttachment( vk::Framebuffer::Attachment( colorAttachmentInternalFormat, textureParams ) );

				// Add to lookup
				uint64_t high = ( static_cast<uint64_t>( subpassIndex ) << 32 ) & 0xFFFFFFFF00000000ULL;
				uint64_t low = ( static_cast<uint64_t>( colorAttachmentIndex ) << 0 ) & 0xFFFFFFFF00000000ULL;
				uint64_t key = high | low;
				std::pair<uint64_t, uint32_t> keyValue = std::make_pair( key, colorAttachmentIndex );
				mColorAttachmentMap.push_back( keyValue );
			}
		}

		VkFormat depthStencilAttachmentInternalFormat = pass.mDepthStencilAttachment;
		if( VK_FORMAT_UNDEFINED != depthStencilAttachmentInternalFormat ) {
			// Process depth attachment(s)
			if( options.mPerSubpassDepthStencil ) {
				// Add depth attachment to attachment list
				attachmentList.push_back( depthStencilAttachmentInternalFormat );

				// Add color attachment to render pass
				vk::RenderPass::Attachment renderPassAttachment = vk::RenderPass::Attachment( depthStencilAttachmentInternalFormat, options.mSamples );
				renderPassAttachment.setInitialAndFinalLayout( VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
				renderPassOptions.addAttachment( renderPassAttachment );

				// Add attachment to subpass
				depthStencilAttachmentIndex = static_cast<uint32_t>( attachmentList.size() - 1 );
				subpass.addDepthStencilAttachment( depthStencilAttachmentIndex );

				// Add attachment to framebuffer
				vk::Texture2d::Format textureParams = options.mDepthStencilTextureParams;
				textureParams.setSamples( options.mSamples );
				framebufferFormat.addAttachment( vk::Framebuffer::Attachment( depthStencilAttachmentInternalFormat, textureParams ) );

				// Add to lookup
				uint32_t key = subpassIndex;
				std::pair<uint32_t, uint32_t> keyValue = std::make_pair( key, depthStencilAttachmentIndex );
				mDepthStencilAttachmentMap.push_back( keyValue );
			}
			else {
				if( UINT32_MAX != depthStencilAttachmentIndex ) {
					// Add attachment to subpass
					subpass.addDepthStencilAttachment( depthStencilAttachmentIndex );

					// Add to lookup
					uint32_t key = subpassIndex;
					std::pair<uint32_t, uint32_t> keyValue = std::make_pair( key, depthStencilAttachmentIndex );
					mDepthStencilAttachmentMap.push_back( keyValue );
				}
				else {
					// Add depth attachment to attachment list
					attachmentList.push_back( depthStencilAttachmentInternalFormat );
				
					// Add color attachment to render pass
					vk::RenderPass::Attachment renderPassAttachment = vk::RenderPass::Attachment( depthStencilAttachmentInternalFormat, options.mSamples );
					renderPassAttachment.setInitialAndFinalLayout( VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
					renderPassOptions.addAttachment( renderPassAttachment );
				
					// Add attachment to subpass
					depthStencilAttachmentIndex = static_cast<uint32_t>( attachmentList.size() - 1 );
					subpass.addDepthStencilAttachment( depthStencilAttachmentIndex );
				
					// Add attachment to framebuffer
					vk::Texture2d::Format textureParams = options.mDepthStencilTextureParams;
					textureParams.setSamples( options.mSamples );
					framebufferFormat.addAttachment( vk::Framebuffer::Attachment( depthStencilAttachmentInternalFormat, textureParams ) );

					// Add to lookup
					uint32_t key = subpassIndex;
					std::pair<uint32_t, uint32_t> keyValue = std::make_pair( key, depthStencilAttachmentIndex );
					mDepthStencilAttachmentMap.push_back( keyValue );
				}
			}
		}

		// Make subpass self dependent
		subpass.setSelfDependent();

		// Add subpass
		renderPassOptions.addSubPass( subpass );				
	}

	mRenderPass = vk::RenderPass::create( renderPassOptions, device );
	mFramebuffer = vk::Framebuffer::create( mRenderPass->getRenderPass(), mSize, framebufferFormat, device );
}

RenderTargetRef RenderTarget::create( const ivec2& size, VkFormat attachment, vk::Device* device )
{
	device = ( nullptr != device ) ? device : vk::Context::getCurrent()->getDevice();
	RenderTarget::Options options = RenderTarget::Options( RenderTarget::Pass( attachment ) );
	RenderTargetRef result = RenderTargetRef( new RenderTarget( size, options, device ) );
	return result;
}

RenderTargetRef RenderTarget::create( const ivec2& size, VkFormat colorAttachment, VkFormat depthStencilAttachment, vk::Device* device )
{
	device = ( nullptr != device ) ? device : vk::Context::getCurrent()->getDevice();
	RenderTarget::Options options = RenderTarget::Options( RenderTarget::Pass( colorAttachment, depthStencilAttachment ) );
	RenderTargetRef result = RenderTargetRef( new RenderTarget( size, options, device ) );
	return result;
}

RenderTargetRef RenderTarget::create( const ivec2& size, const RenderTarget::Options& options, vk::Device* device )
{
	device = ( nullptr != device ) ? device : vk::Context::getCurrent()->getDevice();
	RenderTarget::Options updatedOptions = options;
	if( updatedOptions.mPasses.empty() ) {
		VkFormat colorAttachment = options.mColorTextureParams.getInternalFormat();
		VkFormat depthStencilAttachment = options.mDepthStencilTextureParams.getInternalFormat();
		if( ( VK_FORMAT_UNDEFINED == colorAttachment ) && ( VK_FORMAT_UNDEFINED == depthStencilAttachment ) ) {
			throw std::runtime_error( "Cannot create empty render target" );
		}
		updatedOptions.addPass( RenderTarget::Pass( colorAttachment, depthStencilAttachment ) );
	}
	RenderTargetRef result = RenderTargetRef( new RenderTarget( size, updatedOptions, device ) );
	return result;
}

const vk::Texture2dRef&	RenderTarget::getColorTexture( uint32_t subpassIndex, uint32_t subpassColorAttachmentIndex ) const
{
	uint64_t high = ( static_cast<uint64_t>( subpassIndex ) << 32 ) & 0xFFFFFFFF00000000ULL;
	uint64_t low = ( static_cast<uint64_t>( subpassColorAttachmentIndex ) << 0 ) & 0xFFFFFFFF00000000ULL;
	uint64_t key = high | low;

	uint32_t attachmentIndex = UINT32_MAX;
	// Find attachment index
	{
		auto it = std::find_if( std::begin( mColorAttachmentMap ), std::end( mColorAttachmentMap ),
			[key]( const std::pair<uint64_t, uint32_t>& elem ) -> bool {
				return elem.first == key;
			}
		);

		if( std::end( mColorAttachmentMap ) != it ) {
			attachmentIndex = it->second;
		}
	}
		
	return mFramebuffer->getTexture( attachmentIndex );
}

const vk::Texture2dRef&	RenderTarget::getDepthStencilTexture( uint32_t subpassIndex ) const
{
	uint32_t key = subpassIndex;

	uint32_t attachmentIndex = UINT32_MAX;
	// Find attachment index
	{
		auto it = std::find_if( std::begin( mDepthStencilAttachmentMap ), std::end( mDepthStencilAttachmentMap ),
			[key]( const std::pair<uint32_t, uint32_t>& elem ) -> bool {
				return elem.first == key;
			}
		);

		if( std::end( mDepthStencilAttachmentMap ) != it ) {
			attachmentIndex = it->second;
		}
	}
		
	return mFramebuffer->getTexture( attachmentIndex );
}

void RenderTarget::transitionToShaderReadOnly( const vk::CommandBufferRef& cmdBuf )
{
	if( ( ! mFramebuffer ) || ( ! cmdBuf ) ) {
		return;
	}

	const auto& attachments = mFramebuffer->getAttachments();
	for( const auto& attachment : attachments ) {
		const auto& storage = attachment.getStorage();

		vk::ImageMemoryBarrierParams imageMemoryBarrier = vk::ImageMemoryBarrierParams( storage );
			imageMemoryBarrier.setSrcAndDstStageMask( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
		if( attachment.isColorAttachment() ) {
			imageMemoryBarrier.setOldAndNewLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			imageMemoryBarrier.setSrcAndDstAccessMask( VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT );
		}
		else if( attachment.isDepthStencilAttachment() ) {
			imageMemoryBarrier.setOldAndNewLayout( VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			imageMemoryBarrier.setSrcAndDstStageMask( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT );
			imageMemoryBarrier.setSrcAndDstAccessMask( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT );
		}

		cmdBuf->pipelineBarrierImageMemory( imageMemoryBarrier );
	}
}

void RenderTarget::beginRender( const vk::CommandBufferRef& cmdBuf )
{
	mCommandBuffer = cmdBuf;
	mRenderPass->beginRender( mCommandBuffer, mFramebuffer );
}

void RenderTarget::endRender()
{
	mRenderPass->endRender();
	transitionToShaderReadOnly( mCommandBuffer );
	mCommandBuffer.reset();
}

void RenderTarget::beginRenderExplicit( const vk::CommandBufferRef& cmdBuf )
{
	mCommandBuffer = cmdBuf;
	mRenderPass->beginRenderExplicit( mCommandBuffer, mFramebuffer );
}

void RenderTarget::endRenderExplicit()
{
	mRenderPass->endRenderExplicit();
	transitionToShaderReadOnly( mCommandBuffer );
	mCommandBuffer.reset();
}

void RenderTarget::nextSubpass()
{
	mRenderPass->nextSubpass();
}

}} // namespace cinder::vk