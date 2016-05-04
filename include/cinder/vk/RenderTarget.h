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

#pragma once

#include "cinder/vk/Framebuffer.h"
#include "cinder/vk/RenderPass.h"
#include "cinder/vk/Texture.h"

#include <map>

namespace cinder { namespace vk {

class CommandBuffer;
using CommandBufferRef = std::shared_ptr<CommandBuffer>;

class RenderTarget;
using RenderTargetRef = std::shared_ptr<RenderTarget>;

// \class RenderTarget
//
//
class RenderTarget {
public:

	class Pass {
	public:
		Pass() {}
		Pass( VkFormat attachment );
		Pass( VkFormat colorAttachment, VkFormat depthStencilAttachment );
		Pass( const std::vector<VkFormat>& colorAttachments, VkFormat depthStencilAttachment = VK_FORMAT_UNDEFINED );
		virtual ~Pass() {}
		Pass&					addColorAttachment( VkFormat attachment ) { mColorAttachments.push_back( attachment ); return *this; }
		Pass&					addColorAttachments( const std::vector<VkFormat>& attachments ) { std::copy( std::begin( attachments ), std::end( attachments ), std::back_inserter( mColorAttachments ) ); return *this; }
		Pass&					setDepthStencilAttachment( VkFormat attachment ) { mDepthStencilAttachment = attachment; return *this; }
	private:
		std::vector<VkFormat>	mColorAttachments;
		VkFormat				mDepthStencilAttachment = VK_FORMAT_UNDEFINED;
		friend class RenderTarget;
	};

	// ---------------------------------------------------------------------------------------------

	class Options {
	public:
		Options( VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT ) : mSamples( samples ) {}
		Options( const vk::RenderTarget::Pass& pass, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT );
		virtual ~Options() {}
		Options&						setSamples( VkSampleCountFlagBits value ) { mSamples = value; mColorTextureParams.setSamples( mSamples ); mDepthStencilTextureParams.setSamples( mSamples ); return *this; }
		Options&						setColorTextureParams( const vk::Texture2d::Format& value ) { mColorTextureParams = value; mColorTextureParams.setSamples( mSamples ); return *this; }
		Options&						setColorTextureParams( VkFormat internalFormat ) { mColorTextureParams = vk::Texture2d::Format( internalFormat ); mColorTextureParams.setSamples( mSamples ); return *this; }
		Options&						setDepthStencilTextureParams( const vk::Texture2d::Format& value ) { mDepthStencilTextureParams = value; mDepthStencilTextureParams.setSamples( mSamples ); return *this; }
		Options&						setDepthStencilTextureParams( VkFormat internalFormat ) { mDepthStencilTextureParams = vk::Texture2d::Format( internalFormat ); mDepthStencilTextureParams.setSamples( mSamples ); return *this; }
		Options&						addPass( const vk::RenderTarget::Pass& pass ) { mPasses.push_back( pass ); return *this; }
		Options&						setPerSubpassDepthStencilAttachment( bool value ) { mPerSubpassDepthStencil = value; return *this; }
		Options&						setTransitionToShaderReadOnly( bool value ) { mTransitionToShaderReadOnly = value; return *this; }
	private:
		VkSampleCountFlagBits			mSamples = VK_SAMPLE_COUNT_1_BIT; 
		vk::Texture2d::Format			mColorTextureParams;
		vk::Texture2d::Format			mDepthStencilTextureParams;
		std::vector<RenderTarget::Pass>	mPasses;
		bool							mPerSubpassDepthStencil = false;
		bool							mTransitionToShaderReadOnly = true;
		friend class RenderTarget;
	};

	// ---------------------------------------------------------------------------------------------

	virtual ~RenderTarget();

	static RenderTargetRef		create( const ivec2& size, VkFormat attachment, vk::Device* device = nullptr );
	static RenderTargetRef		create( const ivec2& size, VkFormat colorAttachment, VkFormat depthStencilAttachment, vk::Device* device = nullptr );
	static RenderTargetRef		create( const ivec2& size, const RenderTarget::Options& options, vk::Device* device = nullptr );

	const ivec2&				getSize() const { return mSize; }
	uint32_t					getWidth() const { return static_cast<uint32_t>( mSize.x ); }
	uint32_t					getHeight() const { return static_cast<uint32_t>( mSize.y ); }
	const Rectf&				getBounds() const { return mBounds; }
	float						getAspectRatio() const { return static_cast<float>( mSize.x ) / static_cast<float>( mSize.y ); }

	uint32_t					getSubpassCount() const { return mSubpassCount; }

	const vk::RenderPassRef&	getRenderPass() const { return mRenderPass; }
	const vk::FramebufferRef&	getFramebuffer() const { return mFramebuffer; }

	const vk::Texture2dRef&		getColorTexture( uint32_t subpassIndex = 0, uint32_t subpassColorAttachmentIndex = 0 ) const;
	const vk::Texture2dRef&		getDepthStencilTexture( uint32_t subpassIndex = 0 ) const;

	void						transitionToShaderReadOnly( const vk::CommandBufferRef& cmdBuf );

	void						beginRender( const vk::CommandBufferRef& cmdBuf );
	void						endRender();
	void						beginRenderExplicit( const vk::CommandBufferRef& cmdBuf );
	void						endRenderExplicit();
	void						nextSubpass();

private:
	RenderTarget( const ivec2& size, const RenderTarget::Options& options, vk::Device* device );

	ivec2										mSize;
	Rectf										mBounds;
	uint32_t									mSubpassCount = 0;
	vk::RenderPassRef							mRenderPass;
	vk::FramebufferRef							mFramebuffer;
	std::vector<std::pair<uint64_t, uint32_t>>	mColorAttachmentMap;
	std::vector<std::pair<uint32_t, uint32_t>>	mDepthStencilAttachmentMap;
	vk::CommandBufferRef						mCommandBuffer;

	void initialize( const RenderTarget::Options& options, vk::Device* device );
};

}} // namesapce cinder::v