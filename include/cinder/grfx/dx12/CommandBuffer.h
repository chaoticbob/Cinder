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

class Queue;

// ----------------------------------------------------------------------------------------------------
// CommandBufferShim
// ----------------------------------------------------------------------------------------------------
template <typename BaseT>
class CommandBufferShim : public BaseT {
  public:
	CommandBufferShim( dx12::Queue *pQueue )
		: BaseT( pQueue ) {}

	virtual ~CommandBufferShim() {}

	ID3D12CommandAllocator	  *getCommandAllocator() const { return mCommandAllocator.Get(); }
	ID3D12GraphicsCommandList *getCommandList() const { return mCommandList.Get(); }

  protected:
	ComPtr<ID3D12CommandAllocator>	  mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
};

// ----------------------------------------------------------------------------------------------------
// GraphicsCommandBuffer
// ----------------------------------------------------------------------------------------------------
class GraphicsCommandBuffer : public dx12::CommandBufferShim<grfx::GraphicsCommandBuffer> {
  public:
	GraphicsCommandBuffer( dx12::Queue *pQueue )
		: dx12::CommandBufferShim<grfx::GraphicsCommandBuffer>( pQueue ) {}

	virtual ~GraphicsCommandBuffer() {}

	virtual void submit() override;
	virtual void flush() override;
};

// ----------------------------------------------------------------------------------------------------
// ComputeCommandBuffer
// ----------------------------------------------------------------------------------------------------
class ComputeCommandBuffer : public dx12::CommandBufferShim<grfx::ComputeCommandBuffer> {
  public:
	ComputeCommandBuffer( dx12::Queue *pQueue )
		: dx12::CommandBufferShim<grfx::ComputeCommandBuffer>( pQueue ) {}

	virtual ~ComputeCommandBuffer() {}

	virtual void submit() override;
	virtual void flush() override;
};

// ----------------------------------------------------------------------------------------------------
// CopyCommandBuffer
// ----------------------------------------------------------------------------------------------------
class CopyCommandBuffer : public dx12::CommandBufferShim<grfx::CopyCommandBuffer> {
  public:
	CopyCommandBuffer( dx12::Queue *pQueue )
		: dx12::CommandBufferShim<grfx::CopyCommandBuffer>( pQueue ) {}

	virtual ~CopyCommandBuffer() {}

	virtual void submit() override;
	virtual void flush() override;
};

} // namespace cinder::grfx::dx12

