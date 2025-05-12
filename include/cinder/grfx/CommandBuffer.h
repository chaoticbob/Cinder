/*
 Copyright (c) 2025, The Cinder Project, All rights reserved.

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

#include "cinder/grfx/platform.h"
#include "cinder/grfx/RenderTarget.h"
#include "cinder/Color.h"

namespace cinder::grfx {

class CommandBuffer;
class Queue;
class RenderTarget;
class DepthStencil;

using CommandBufferRef = std::shared_ptr<grfx::CommandBuffer>;
using QueueRef		   = std::shared_ptr<grfx::Queue>;
using RenderTargetRef  = std::shared_ptr<grfx::RenderTarget>;
using DepthStencilRef  = std::shared_ptr<grfx::DepthStencil>;

enum class BeginOp
{
	CLEAR	  = 0,
	LOAD	  = 1,
	OVERWRITE = 2,
};

enum class EndOp
{
	STORE	= 1,
	DISCARD = 2,
};

class ColorAttachment {
  public:
	ColorAttachment( const grfx::RenderTargetRef &renderTarget = nullptr )
		: mRenderTarget( renderTarget ) {}
	~ColorAttachment() {}

	const grfx::RenderTargetRef &renderTarget() const { return mRenderTarget; }
	grfx::BeginOp				 beginOp() const { return mBeginOp; }
	bool						 isClear() const { return ( mBeginOp == grfx::BeginOp::CLEAR ); }
	bool						 isLoad() const { return ( mBeginOp == grfx::BeginOp::LOAD ); }
	bool						 isOverwrite() const { return ( mBeginOp == grfx::BeginOp::OVERWRITE ); }
	grfx::EndOp					 endOp() const { return mEndOp; }
	bool						 isStore() const { return ( mEndOp == grfx::EndOp::STORE ); }
	bool						 isDiscard() const { return ( mEndOp == grfx::EndOp::DISCARD ); }
	const ColorAf				&clearColor() const { return mClearColor; }

	// clang-format off
	grfx::ColorAttachment &renderTarget(const grfx::RenderTargetRef& renderTarget) { mRenderTarget = renderTarget; return *this; }

	grfx::ColorAttachment &beginOp(grfx::BeginOp op, const ColorAf& clearColor = ColorAf()) { mBeginOp = op; mClearColor = clearColor; return *this; }
	grfx::ColorAttachment &clear(const ColorAf& clearColor) { mBeginOp = grfx::BeginOp::CLEAR; mClearColor = clearColor; return *this; }
	grfx::ColorAttachment &load() { mBeginOp = grfx::BeginOp::LOAD; return *this; }
	grfx::ColorAttachment &overwrite() { mBeginOp = grfx::BeginOp::OVERWRITE; return *this; }
	
	grfx::ColorAttachment &endOp(grfx::EndOp op) { mEndOp = op; return *this; }
	grfx::ColorAttachment &store() { mEndOp = grfx::EndOp::STORE; return *this; }
	grfx::ColorAttachment &discard() { mEndOp = grfx::EndOp::DISCARD; return *this; }
	// clang-format on

  private:
	grfx::RenderTargetRef mRenderTarget = nullptr;
	grfx::BeginOp		  mBeginOp		= grfx::BeginOp::CLEAR;
	grfx::EndOp			  mEndOp		= grfx::EndOp::STORE;
	ColorAf				  mClearColor	= ColorAf();
};

class RenderPass {
  public:
	RenderPass() {}
	RenderPass( const std::vector<grfx::ColorAttachment> &colorAttachments )
		: mColorAttachments( colorAttachments ) {}
	~RenderPass() {}

	const std::vector<grfx::ColorAttachment> &getColorAttachments() const { return mColorAttachments; }

	// clang-format off
	grfx::RenderPass& addColorAttachment(const grfx::ColorAttachment& desc) { mColorAttachments.push_back(desc); return *this; }
	// clang-format on

  private:
	std::vector<grfx::ColorAttachment> mColorAttachments = {};
};

// ----------------------------------------------------------------------------------------------------
// CommandBuffer
// ----------------------------------------------------------------------------------------------------
class CommandBuffer : public grfx::DeviceChild {
  public:
	CommandBuffer( grfx::Queue *pParentQueue );
	virtual ~CommandBuffer() {}

	grfx::Queue *getQueue() const { return mQueue; }

	// Submits command buffer to queue for execution.
	virtual void submit() = 0;
	// Submits command buffer to queue for execution, waits until execution completes.
	virtual void flush() = 0;

	virtual void reset() = 0;
	virtual void close() = 0;

	virtual void beginRenderPass( const grfx::RenderPass &renderPass ) = 0;
	virtual void endRenderPass()									   = 0;

	virtual void resolveSubresource( const grfx::Texture2D *pDstTexture, uint32_t dstSubResourceIndex, const grfx::Texture2D *pSrcTexture, uint32_t srcSubResourceIndex ) = 0;
	void		 resolveSubresource( const grfx::Texture2D *pDstTexture, uint32_t dstSubResourceIndex, const grfx::RenderTarget *pSrcRenderTarget, uint32_t srcSubResourceIndex );

  private:
	grfx::Queue *mQueue = nullptr;
};

} // namespace cinder::grfx
