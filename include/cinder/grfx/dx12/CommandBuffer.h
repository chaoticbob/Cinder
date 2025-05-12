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

class CommandBuffer;
class Queue;

using CommandBufferRef = std::shared_ptr<dx12::CommandBuffer>;
using QueueRef		   = std::shared_ptr<dx12::Queue>;

// ----------------------------------------------------------------------------------------------------
// CommandBuffer
// ----------------------------------------------------------------------------------------------------
class CommandBuffer : public dx12::DeviceChildShim<grfx::CommandBuffer> {
  public:
	CommandBuffer( dx12::Queue *pParentQueue );
	virtual ~CommandBuffer();

	ID3D12CommandAllocator	   *getD3D12CommandAllocator() const { return mCommandAllocator.Get(); }
	ID3D12GraphicsCommandList4 *getD3D12CommandList() const { return mCommandList.Get(); }

	virtual void submit() override;
	virtual void flush() override;

	virtual void reset() override;
	virtual void close() override;

	virtual void beginRenderPass( const grfx::RenderPass &renderPass ) override;
	virtual void endRenderPass() override;

	virtual void resolveSubresource( const grfx::Texture2D *pDstTexture, uint32_t dstSubResourceIndex, const grfx::Texture2D *pSrcTexture, uint32_t srcSubResourceIndex ) override;

  private:
	ComPtr<ID3D12CommandAllocator>	   mCommandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList4> mCommandList		 = nullptr;
};

} // namespace cinder::grfx::dx12

