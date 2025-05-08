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

class Queue;

using QueueRef = std::shared_ptr<dx12::Queue>;

class Queue : public dx12::DeviceChildShim<cinder::grfx::Queue> {
  public:
	Queue( dx12::Device *pDevice, grfx::CommandType commandType );
	virtual ~Queue();

	ID3D12CommandQueue *getD3D12CommandQueue() const { return mCommandQueue.Get(); }

	virtual void waitForIdle() override;

	virtual grfx::GraphicsCommandBufferRef createGraphicsCommandBuffer() override;
	virtual grfx::ComputeCommandBufferRef  createComputeCommandBuffer() override;
	virtual grfx::CopyCommandBufferRef	   createCopyCommandBuffer() override;

  private:
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	std::mutex				   mWaitForIdleMutex;
	ComPtr<ID3D12Fence>		   mWaitForIdleFence;
	uint64_t				   mWaitForIdleValue = 0;
	HANDLE					   mWaitForIdleEvent = nullptr;
};

} // namespace cinder::grfx::dx12
