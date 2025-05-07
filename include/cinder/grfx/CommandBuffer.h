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

namespace cinder::grfx {

class GraphicsCommandBuffer;
class ComputeCommandBuffer;
class CopyCommandBuffer;
class Queue;

using GraphicsCommandBufferRef = std::shared_ptr<grfx::GraphicsCommandBuffer>;
using ComputeCommandBufferRef  = std::shared_ptr<grfx::ComputeCommandBuffer>;
using CopyCommandBufferRef	   = std::shared_ptr<grfx::CopyCommandBuffer>;

// ----------------------------------------------------------------------------------------------------
// CommandBufferBase
// ----------------------------------------------------------------------------------------------------
class CommandBufferBase {
  public:
	CommandBufferBase() {}
	virtual ~CommandBufferBase() {}

	virtual grfx::Queue *getQueue() const = 0;
	virtual void		 submit()		  = 0;
	virtual void		 flush()		  = 0;
};

// ----------------------------------------------------------------------------------------------------
// GraphicsCommandBuffer
// ----------------------------------------------------------------------------------------------------
class GraphicsCommandBuffer : public grfx::CommandBufferBase {
  public:
	GraphicsCommandBuffer() {}
	virtual ~GraphicsCommandBuffer() {}

	virtual void BeginRendering( const std::vector<grfx::RenderTargetRef> &renderTargets, grfx::DepthStencilRef &depthStencil = grfx::DepthStencilRef() ) = 0;
	virtual void EndRendering()																															  = 0;
};

// ----------------------------------------------------------------------------------------------------
// ComputeCommandBuffer
// ----------------------------------------------------------------------------------------------------
class ComputeCommandBuffer : public grfx::CommandBufferBase {
  public:
	ComputeCommandBuffer() {}
	virtual ~ComputeCommandBuffer() {}
};

// ----------------------------------------------------------------------------------------------------
// CopyCommandBuffer
// ----------------------------------------------------------------------------------------------------
class CopyCommandBuffer : public grfx::CommandBufferBase {
  public:
	CopyCommandBuffer() {}
	virtual ~CopyCommandBuffer() {}
};

} // namespace cinder::grfx

