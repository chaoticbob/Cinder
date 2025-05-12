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

// ----------------------------------------------------------------------------------------------------
// DescriptorHeap
// ----------------------------------------------------------------------------------------------------
DescriptorHeap::DescriptorHeap( dx12::Device *pDevice, uint32_t descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE type )
	: dx12::DeviceChildShim<grfx::DeviceChild>( pDevice )
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type						= type;
	desc.NumDescriptors				= static_cast<UINT>( descriptorCount );
	desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask					= 0;

	HRESULT hr = this->getD3D12Device()->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &mHeap ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 descriptor heap" );
	}
}

DescriptorHeap::~DescriptorHeap()
{
}

uint32_t DescriptorHeap::getDescriptorCount() const
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = mHeap->GetDesc();
	return static_cast<uint32_t>( desc.NumDescriptors );
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeap::getType() const
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = mHeap->GetDesc();
	return desc.Type;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getHeapStart() const
{
	return mHeap->GetCPUDescriptorHandleForHeapStart();
}

// ----------------------------------------------------------------------------------------------------
// FixedSizeDescriptorHeap
// ----------------------------------------------------------------------------------------------------
FixedSizeDescriptorHeap::FixedSizeDescriptorHeap( dx12::Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type )
	: dx12::DescriptorHeap( pDevice, dx12::FixedSizeDescriptorHeap::kSetSize, type )
{
}

FixedSizeDescriptorHeap::~FixedSizeDescriptorHeap()
{
}

dx12::CpuDescriptorHandle FixedSizeDescriptorHeap::allocateHandle()
{
	dx12::CpuDescriptorHandle handle = {};

	uint32_t index = 0;
	for( ; index < kSetSize; ++index ) {
		if( mBitset[index] == false ) {
			break;
		}
	}

	if( index < kSetSize ) {
		mBitset[index] = true;

		const D3D12_CPU_DESCRIPTOR_HANDLE heapStart		   = this->getHeapStart();
		const uint32_t					  handleStride	   = this->getDevice()->getCpuDescriptorHandleStride( this->getType() );
		const D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = { heapStart.ptr + static_cast<SIZE_T>( index * handleStride ) };
		handle											   = dx12::CpuDescriptorHandle( this, descriptorHandle );
	}

	return handle;
}

void FixedSizeDescriptorHeap::freeHandle( const dx12::CpuDescriptorHandle &handle )
{
	const dx12::DescriptorHeap		 *pHeap		   = handle.getHeap();
	const D3D12_CPU_DESCRIPTOR_HANDLE heapStart	   = pHeap->getHeapStart();
	const uint32_t					  handleStride = this->getDevice()->getCpuDescriptorHandleStride( this->getType() );
	const SIZE_T					  offset	   = handle.getD3D12Handle().ptr - heapStart.ptr;
	const uint32_t					  index		   = static_cast<uint32_t>( offset / handleStride );

	mBitset[index] = false;
}

// ----------------------------------------------------------------------------------------------------
// DsvDescriptorHeap
// ----------------------------------------------------------------------------------------------------
FixedSizedDescriptorHeapManager::FixedSizedDescriptorHeapManager( dx12::Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type )
	: dx12::DeviceChildShim<grfx::DeviceChild>( pDevice ),
	  mType( type )
{
}

FixedSizedDescriptorHeapManager::~FixedSizedDescriptorHeapManager()
{
}

dx12::CpuDescriptorHandle FixedSizedDescriptorHeapManager::allocateHandle()
{
	dx12::CpuDescriptorHandle handle = {};

	for( auto &heap : mHeaps ) {
		handle = heap->allocateHandle();
		if( handle ) {
			break;
		}
	}

	if( ! handle ) {
		auto heap = std::shared_ptr<FixedSizeDescriptorHeap>( new dx12::FixedSizeDescriptorHeap( this->getDevice(), this->mType ) );
		handle	  = heap->allocateHandle();
		mHeaps.push_back( heap );
	}

	return handle;
}

void FixedSizedDescriptorHeapManager::freeHandle( const dx12::CpuDescriptorHandle &handle )
{
	dx12::FixedSizeDescriptorHeap *pHeap = nullptr;
	for( auto &heap : mHeaps ) {
		if( handle.getHeap() == heap.get() ) {
			pHeap = heap.get();
			break;
		}
	}

	if( pHeap == nullptr ) {
		throw cinder::Exception( "Descriptor handle does not belong any heap" );
	}

	pHeap->freeHandle( handle );
}

// ----------------------------------------------------------------------------------------------------
// Device
// ----------------------------------------------------------------------------------------------------
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

	mRtvDescriptorHeapManager = std::make_shared<dx12::FixedSizedDescriptorHeapManager>( this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
	mDsvDescriptorHeapManager = std::make_shared<dx12::FixedSizedDescriptorHeapManager>( this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV );
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

grfx::FenceRef Device::createFence( uint64_t initialValue )
{
	return dx12::FenceRef( new dx12::Fence( this, initialValue ) );
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

UINT Device::getCpuDescriptorHandleStride( D3D12_DESCRIPTOR_HEAP_TYPE type ) const
{
	UINT stride = mDevice->GetDescriptorHandleIncrementSize( type );
	return stride;
}

dx12::CpuDescriptorHandle Device::allocateHandle( D3D12_DESCRIPTOR_HEAP_TYPE type )
{
	dx12::CpuDescriptorHandle handle = {};
	switch( type ) {
		default: {
			throw cinder::Exception( "Unsupported D3D12 descriptor heap type" );
		} break;

		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV: handle = mRtvDescriptorHeapManager->allocateHandle(); break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV: handle = mDsvDescriptorHeapManager->allocateHandle(); break;
	}
	return handle;
}

void Device::freeHandle( const dx12::CpuDescriptorHandle &handle )
{
	auto pBaseHeap = dynamic_cast<const dx12::FixedSizeDescriptorHeap *>( handle.getHeap() );
	if( pBaseHeap ) {
		throw cinder::Exception( "CPU descriptor handle was not allocated from a FixedSizedDescriptorHeap" );
	}

	auto pHeap = const_cast<dx12::FixedSizeDescriptorHeap *>( pBaseHeap );
	pHeap->freeHandle( handle );
}

} // namespace cinder::grfx::dx12
