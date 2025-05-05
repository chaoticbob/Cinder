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

namespace cinder::grfx::dx12 {

Queue::Queue( const ComPtr<ID3D12CommandQueue> &queue )
	: mQueue( queue ),
	  mWaitForIdleEvent( CreateEvent( nullptr, FALSE, FALSE, nullptr ) )
{
}

Queue::~Queue()
{
	this->waitForIdle();

	if( mWaitForIdleEvent ) {
		CloseHandle( mWaitForIdleEvent );
	}
}

void Queue::initialize( ID3D12Device *pDevice )
{
	HRESULT hr = pDevice->CreateFence( mWaitForIdleValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &mWaitForIdleFence ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create fence DX12 queue" );
	}
}

QueueRef Queue::create( ID3D12Device *pDevice, D3D12_COMMAND_LIST_TYPE commandType )
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type					  = commandType;
	desc.Priority				  = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask				  = 0;

	ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	//
	HRESULT hr = pDevice->CreateCommandQueue( &desc, IID_PPV_ARGS( &commandQueue ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 command queue" );
	}

	QueueRef queue = QueueRef( new Queue( commandQueue ) );
	queue->initialize( pDevice );

	return queue;
}

void Queue::waitForIdle()
{
	std::lock_guard<std::mutex> lock( mWaitForIdleMutex );

	++mWaitForIdleValue;

	HRESULT hr = mQueue->Signal( mWaitForIdleFence.Get(), mWaitForIdleValue );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to queue a sigal" );
	}

	UINT64 completedValue = mWaitForIdleFence->GetCompletedValue();
	if( completedValue < mWaitForIdleValue ) {
		mWaitForIdleFence->SetEventOnCompletion( mWaitForIdleValue, mWaitForIdleEvent );
		WaitForSingleObject( mWaitForIdleEvent, INFINITE );
	}
}

} // namespace cinder::grfx::dx12
