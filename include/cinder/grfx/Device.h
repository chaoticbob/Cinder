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
#include "cinder/Noncopyable.h"

namespace cinder::grfx {

class Fence;
class Queue;

using FenceRef = std::shared_ptr<grfx::Fence>;
using QueueRef = std::shared_ptr<grfx::Queue>;

class Device : public cinder::Noncopyable {
  public:
	Device() {}
	virtual ~Device() {}

	virtual void waitForIdle() = 0;

	virtual grfx::FenceRef createFence( uint64_t initialValue = 0 ) = 0;

	grfx::Queue *getGraphicsQueue() const { return mGraphicsQueue.get(); }
	grfx::Queue *getComputeQueue() const { return mComputeQueue.get(); }
	grfx::Queue *getCopyQueue() const { return mCopyQueue.get(); }

  protected:
	grfx::QueueRef mGraphicsQueue = nullptr;
	grfx::QueueRef mComputeQueue  = nullptr;
	grfx::QueueRef mCopyQueue	  = nullptr;
};

} // namespace cinder::grfx
