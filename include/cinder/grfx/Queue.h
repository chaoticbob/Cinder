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

namespace cinder::grfx {

class GraphicsCommandBuffer;
class ComputeCommandBuffer;
class CopyCommandBuffer;
class Queue;

using GraphicsCommandBufferRef = std::shared_ptr<grfx::GraphicsCommandBuffer>;
using ComputeCommandBufferRef  = std::shared_ptr<grfx::ComputeCommandBuffer>;
using CopyCommandBufferRef	   = std::shared_ptr<grfx::CopyCommandBuffer>;
using QueueRef				   = std::shared_ptr<grfx::Queue>;

class Queue {
  public:
	Queue() {}
	virtual ~Queue() {}

	virtual void waitForIdle() = 0;

	virtual grfx::GraphicsCommandBufferRef createGraphicsCommandBuffer() = 0;
	virtual grfx::ComputeCommandBufferRef  createComputeCommandBuffer()	 = 0;
	virtual grfx::CopyCommandBufferRef	   createCopyCommandBuffer()	 = 0;
};

} // namespace cinder::grfx
