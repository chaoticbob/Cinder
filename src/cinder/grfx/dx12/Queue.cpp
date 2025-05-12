/*
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

#include "cinder/grfx/dx12/Queue.h"
#include "cinder/grfx/dx12/CommandBuffer.h"
#include "cinder/grfx/dx12/Device.h"

namespace cinder::grfx::dx12 {

// ----------------------------------------------------------------------------------------------------
// Fence
// ----------------------------------------------------------------------------------------------------
Fence::Fence( dx12::Device *pParentDevice, uint64_t initialValue )
	: dx12::DeviceChildShim<grfx::Fence>( pParentDevice ),
	  mWaitEvent( CreateEvent( nullptr, FALSE, FALSE, nullptr ) )
{
	HRESULT hr = this->getD3D12Device()->CreateFence(
		static_cast<UINT64>( initialValue ),
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS( &mFence ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 fence" );
	}
}

Fence::~Fence()
{
	if( mWaitEvent ) {
		CloseHandle( mWaitEvent );
	}

	mFence.Reset();
}

void Fence::signal( uint64_t value )
{
	HRESULT hr = mFence->Signal( static_cast<UINT64>( value ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to signal D3D12 fence" );
	}
}

void Fence::wait( uint64_t value )
{
	UINT64 completedValue = mFence->GetCompletedValue();
	if( completedValue < value ) {
		mFence->SetEventOnCompletion( value, mWaitEvent );
		WaitForSingleObject( mWaitEvent, INFINITE );
	}
}

uint64_t Fence::currentValuue() const
{
	return static_cast<uint64_t>( mFence->GetCompletedValue() );
}

// ----------------------------------------------------------------------------------------------------
// Queue
// ----------------------------------------------------------------------------------------------------
Queue::Queue( dx12::Device *pParentDevice, grfx::CommandType commandType )
	: dx12::DeviceChildShim<cinder::grfx::Queue>( pParentDevice, commandType )
{
	// D3D12 queue
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type					  = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority				  = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask				  = 0;

		if( commandType == grfx::CommandType::COMPUTE ) {
			desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		}
		else if( commandType == grfx::CommandType::COPY ) {
			desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		}

		HRESULT hr = pParentDevice->getD3D12Device()->CreateCommandQueue( &desc, IID_PPV_ARGS( &mCommandQueue ) );
		if( FAILED( hr ) ) {
			throw cinder::Exception( "Failed to create D3D12 command queue" );
		}
	}

	// Wait for idle fence
	{
		mWaitForIdleFence = std::static_pointer_cast<dx12::Fence>( this->getDevice()->createFence() );
	}
}

Queue::~Queue()
{
	this->waitForIdle();

	mWaitForIdleFence.reset();
	mCommandQueue.Reset();
}

void Queue::submit( const std::vector<grfx::CommandBufferRef> &commandBuffers )
{
	std::vector<ID3D12CommandList *> commandLists;
	for( const auto &elem : commandBuffers ) {
		ID3D12CommandList *const pCommandList = std::static_pointer_cast<dx12::CommandBuffer>( elem )->getD3D12CommandList();
		commandLists.push_back( pCommandList );
	}

	mCommandQueue->ExecuteCommandLists( static_cast<UINT>( commandLists.size() ), commandLists.empty() ? nullptr : commandLists.data() );
}

void Queue::flush( const std::vector<grfx::CommandBufferRef> &commandBuffers )
{
	++mFlushCounter;
}

void Queue::signal( const grfx::FenceRef &fence, uint64_t value )
{
	ID3D12Fence *pFence = std::static_pointer_cast<dx12::Fence>( fence )->getD3D12Fence();

	HRESULT hr = mCommandQueue->Signal( pFence, static_cast<uint64_t>( value ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to queue a sigal" );
	}
}

void Queue::wait( const grfx::FenceRef &fence, uint64_t value )
{
	ID3D12Fence *pFence = std::static_pointer_cast<dx12::Fence>( fence )->getD3D12Fence();

	HRESULT hr = mCommandQueue->Wait( pFence, static_cast<uint64_t>( value ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to wait on a sigal" );
	}
}

void Queue::waitForIdle()
{
	std::lock_guard<std::mutex> lock( mWaitForIdleMutex );

	++mWaitForIdleValue;

	HRESULT hr = mCommandQueue->Signal( mWaitForIdleFence->getD3D12Fence(), mWaitForIdleValue );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to queue a sigal" );
	}

	mWaitForIdleFence->wait( mWaitForIdleValue );
}

grfx::CommandBufferRef Queue::createCommandBuffer()
{
	return dx12::CommandBufferRef( new dx12::CommandBuffer( this ) );
}

} // namespace cinder::grfx::dx12
