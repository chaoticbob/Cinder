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

#include "cinder/grfx/dx12/Device.h"
#include "cinder/grfx/dx12/Queue.h"

namespace cinder::grfx::dx12 {

Device::Device(
	const ComPtr<ID3D12Device9> &device,
	bool						 enableGraphicsQueue,
	bool						 enableComputeQueue,
	bool						 enableCopyQueue )
	: mDevice( device )
{
	if( enableGraphicsQueue ) {
		mGraphicsQueue = dx12::QueueRef( new dx12::Queue( this, cinder::grfx::CommandType::GRAPHICS ) );
		mComputeQueue  = mGraphicsQueue;
		mCopyQueue	   = mComputeQueue;
	}

	if( enableComputeQueue ) {
		mGraphicsQueue = dx12::QueueRef( new dx12::Queue( this, cinder::grfx::CommandType::COMPUTE ) );
		mCopyQueue	   = mComputeQueue;
	}

	if( enableCopyQueue ) {
		mCopyQueue = dx12::QueueRef( new dx12::Queue( this, cinder::grfx::CommandType::COPY ) );
	}
}

void Device::waitForIdle()
{
	if( this->getGraphicsQueue() != nullptr ) {
		this->getGraphicsQueue()->waitForIdle();
	}

	if( ( this->getComputeQueue() != nullptr ) && ( this->getComputeQueue() != this->getGraphicsQueue() ) ) {
		this->getComputeQueue()->waitForIdle();
	}

	if( ( this->getCopyQueue() != nullptr ) && ( ( this->getCopyQueue() != this->getGraphicsQueue() ) || ( this->getCopyQueue() != this->getComputeQueue() ) ) ) {
		this->getCopyQueue()->waitForIdle();
	}
}

dx12::Queue *Device::getGraphicsQueue() const
{
	return static_cast<dx12::Queue *>( grfx::Device::getGraphicsQueue() );
}

dx12::Queue *Device::getComputeQueue() const
{
	return static_cast<dx12::Queue *>( grfx::Device::getComputeQueue() );
}

dx12::Queue *Device::getCopyQueue() const
{
	return static_cast<dx12::Queue *>( grfx::Device::getCopyQueue() );
}

} // namespace cinder::grfx::dx12
