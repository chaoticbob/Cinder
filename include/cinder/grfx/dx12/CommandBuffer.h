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

#include "cinder/grfx/dx12/platform.h"
#include "cinder/grfx/CommandBuffer.h"

namespace cinder::grfx::dx12 {

class GraphicsCommandBuffer;
class ComputeCommandBuffer;
class CopyCommandBuffer;
class Queue;

using GraphicsCommandBufferRef = std::shared_ptr<dx12::GraphicsCommandBuffer>;
using ComputeCommandBufferRef  = std::shared_ptr<dx12::ComputeCommandBuffer>;
using CopyCommandBufferRef	   = std::shared_ptr<dx12::CopyCommandBuffer>;

// ----------------------------------------------------------------------------------------------------
// CommandBufferBaseImpl
// ----------------------------------------------------------------------------------------------------
class CommandBufferBaseImpl {
  public:
	CommandBufferBaseImpl( dx12::Queue *pParentQueue );
	virtual ~CommandBufferBaseImpl();

	ID3D12CommandAllocator	   *getD3D12CommandAllocator() const { return mCommandAllocator.Get(); }
	ID3D12GraphicsCommandList4 *getD3D12CommandList() const { return mCommandList.Get(); }

  protected:
	virtual dx12::Queue *getParentQueue() = 0;

	void submitCommands();
	void flushCommands();

  private:
	ComPtr<ID3D12CommandAllocator>	   mCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList4> mCommandList		 = nullptr;
};

// ----------------------------------------------------------------------------------------------------
// CommandBufferShim
// ----------------------------------------------------------------------------------------------------
template <typename BaseT>
class CommandBufferShim : public dx12::DeviceChildShim<BaseT>, public dx12::CommandBufferBaseImpl {
  public:
	CommandBufferShim( dx12::Queue *pParentQueue )
		: dx12::DeviceChildShim<BaseT>( pParentQueue ),
		  dx12::CommandBufferBaseImpl( pParentQueue ) {}

	virtual ~CommandBufferShim() {}

	dx12::Queue *getQueue() const { return static_cast<dx12::Queue *>( BaseT::getQueue() ); }

	virtual void submit() override { this->submitCommands(); }
	virtual void flush() override { this->flushCommands(); }

  protected:
	virtual dx12::Queue *getParentQueue() override { return this->getQueue(); }
};

// ----------------------------------------------------------------------------------------------------
// GraphicsCommandBuffer
// ----------------------------------------------------------------------------------------------------
class GraphicsCommandBuffer : public dx12::CommandBufferShim<grfx::GraphicsCommandBuffer> {
  public:
	GraphicsCommandBuffer( dx12::Queue *pParentQueue )
		: dx12::CommandBufferShim<grfx::GraphicsCommandBuffer>( pParentQueue ) {}

	virtual ~GraphicsCommandBuffer() {}

	virtual void Reset() override;
	virtual void Close() override;

	virtual void BeginRendering( const std::vector<grfx::RenderTargetRef> &renderTargets, grfx::DepthStencilRef &depthStencil = grfx::DepthStencilRef() ) override;
	virtual void EndRendering() override;

	virtual void ClearRenderTarget( uint32_t renderTargetIndex, float r = 0, float g = 0, float b = 0, float a = 0 ) override;

	virtual void ResolveSubresource( const grfx::Texture2D *pDstTexture, uint32_t dstSubResourceIndex, const grfx::Texture2D *pSrcTexture, uint32_t srcSubResourceIndex ) override;
};

// ----------------------------------------------------------------------------------------------------
// ComputeCommandBuffer
// ----------------------------------------------------------------------------------------------------
class ComputeCommandBuffer : public dx12::CommandBufferShim<grfx::ComputeCommandBuffer> {
  public:
	ComputeCommandBuffer( dx12::Queue *pParentQueue )
		: dx12::CommandBufferShim<grfx::ComputeCommandBuffer>( pParentQueue ) {}

	virtual ~ComputeCommandBuffer() {}
};

// ----------------------------------------------------------------------------------------------------
// CopyCommandBuffer
// ----------------------------------------------------------------------------------------------------
class CopyCommandBuffer : public dx12::CommandBufferShim<grfx::CopyCommandBuffer> {
  public:
	CopyCommandBuffer( dx12::Queue *pParentQueue )
		: dx12::CommandBufferShim<grfx::CopyCommandBuffer>( pParentQueue ) {}

	virtual ~CopyCommandBuffer() {}
};

} // namespace cinder::grfx::dx12

