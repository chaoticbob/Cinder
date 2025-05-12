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
#include "cinder/grfx/Queue.h"

#include <mutex>

namespace cinder::grfx::dx12 {

class Fence;
class Queue;

using FenceRef = std::shared_ptr<dx12::Fence>;
using QueueRef = std::shared_ptr<dx12::Queue>;

// ----------------------------------------------------------------------------------------------------
// Fence
// ----------------------------------------------------------------------------------------------------
class Fence : public dx12::DeviceChildShim<grfx::Fence> {
  public:
	Fence( dx12::Device *pParentDevice, uint64_t initialValue = 0 );
	virtual ~Fence();

	ID3D12Fence *getD3D12Fence() const { return mFence.Get(); }

	virtual void	 signal( uint64_t value ) override;
	virtual void	 wait( uint64_t value ) override;
	virtual uint64_t currentValuue() const override;

  private:
	ComPtr<ID3D12Fence> mFence	   = nullptr;
	HANDLE				mWaitEvent = nullptr;
};

// ----------------------------------------------------------------------------------------------------
// Queue
// ----------------------------------------------------------------------------------------------------
class Queue : public dx12::DeviceChildShim<cinder::grfx::Queue> {
  public:
	Queue( dx12::Device *pParentDevice, grfx::CommandType commandType );
	virtual ~Queue();

	ID3D12CommandQueue *getD3D12CommandQueue() const { return mCommandQueue.Get(); }

	virtual void submit( const std::vector<grfx::CommandBufferRef> &commandBuffers ) override;
	virtual void flush( const std::vector<grfx::CommandBufferRef> &commandBuffers ) override;
	virtual void signal( const grfx::FenceRef &fence, uint64_t value ) override;
	virtual void wait( const grfx::FenceRef &fence, uint64_t value ) override;
	virtual void waitForIdle() override;

	virtual grfx::CommandBufferRef createCommandBuffer() override;

  private:
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	std::mutex				   mWaitForIdleMutex;
	dx12::FenceRef			   mWaitForIdleFence;
	uint64_t				   mWaitForIdleValue = 0;
	uint64_t				   mFlushCounter	 = 0;
};

} // namespace cinder::grfx::dx12
