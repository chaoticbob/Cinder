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
#include "cinder/grfx/Device.h"

#include <bitset>

namespace cinder::grfx::dx12 {

class Device;
class Queue;

using DeviceRef = std::shared_ptr<dx12::Device>;

// ----------------------------------------------------------------------------------------------------
// DescriptorHeap
// ----------------------------------------------------------------------------------------------------
class DescriptorHeap : public dx12::DeviceChildShim<grfx::DeviceChild> {
  public:
	DescriptorHeap( dx12::Device *pDevice, uint32_t descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE type );
	virtual ~DescriptorHeap();

	uint32_t					getDescriptorCount() const;
	D3D12_DESCRIPTOR_HEAP_TYPE	getType() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getHeapStart() const;

  private:
	ComPtr<ID3D12DescriptorHeap> mHeap = nullptr;
};

// ----------------------------------------------------------------------------------------------------
// FixedSizeDescriptorHeap
// ----------------------------------------------------------------------------------------------------
class FixedSizeDescriptorHeap : public dx12::DescriptorHeap {
  public:
	static const size_t kSetSize = 128;

	FixedSizeDescriptorHeap( dx12::Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type );
	virtual ~FixedSizeDescriptorHeap();

	// virtual uint32_t getDescriptorHandleStride() const = 0;

	dx12::CpuDescriptorHandle allocateHandle();
	void					  freeHandle( const dx12::CpuDescriptorHandle &handle );

  private:
	std::bitset<kSetSize> mBitset = {};
};

/*
// ----------------------------------------------------------------------------------------------------
// RtvDescriptorHeap
// ----------------------------------------------------------------------------------------------------
class RtvDescriptorHeap : public FixedSizeDescriptorHeap {
  public:
	RtvDescriptorHeap( dx12::Device *pDevice )
		: FixedSizeDescriptorHeap( pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV ) {}
	virtual ~RtvDescriptorHeap() {}

	virtual uint32_t getDescriptorHandleStride() const override;
};

// ----------------------------------------------------------------------------------------------------
// DsvDescriptorHeap
// ----------------------------------------------------------------------------------------------------
class DsvDescriptorHeap : public FixedSizeDescriptorHeap {
  public:
	DsvDescriptorHeap( dx12::Device *pDevice )
		: FixedSizeDescriptorHeap( pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV ) {}
	virtual ~DsvDescriptorHeap() {}

	virtual uint32_t getDescriptorHandleStride() const override;
};
*/

// ----------------------------------------------------------------------------------------------------
// FixedSizedDescriptorHeapManager
// ----------------------------------------------------------------------------------------------------
class FixedSizedDescriptorHeapManager : public dx12::DeviceChildShim<grfx::DeviceChild> {
  public:
	FixedSizedDescriptorHeapManager( dx12::Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type );
	virtual ~FixedSizedDescriptorHeapManager();

	dx12::CpuDescriptorHandle allocateHandle();
	void					  freeHandle( const dx12::CpuDescriptorHandle &handle );

  private:
	D3D12_DESCRIPTOR_HEAP_TYPE									mType = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>( ~0 );
	std::vector<std::shared_ptr<dx12::FixedSizeDescriptorHeap>> mHeaps;
};

// ----------------------------------------------------------------------------------------------------
// Device
// ----------------------------------------------------------------------------------------------------
class Device : public grfx::Device {
  public:
	Device(
		const ComPtr<ID3D12Device9> &device,
		bool						 enableGraphicsQueue,
		bool						 enableComputeQueue,
		bool						 enableCopyQueue );

	virtual ~Device() {}

	ID3D12Device9 *getD3D12Device() const { return mDevice.Get(); }

	virtual void waitForIdle() override;

	virtual grfx::FenceRef createFence( uint64_t initialValue = 0 ) override;

	dx12::Queue *getGraphicsQueue() const;
	dx12::Queue *getComputeQueue() const;
	dx12::Queue *getCopyQueue() const;

	UINT getCpuDescriptorHandleStride( D3D12_DESCRIPTOR_HEAP_TYPE type ) const;

	dx12::CpuDescriptorHandle allocateHandle( D3D12_DESCRIPTOR_HEAP_TYPE type );
	void					  freeHandle( const dx12::CpuDescriptorHandle &handle );

  private:
	ComPtr<ID3D12Device9>								   mDevice					 = nullptr;
	std::shared_ptr<dx12::FixedSizedDescriptorHeapManager> mRtvDescriptorHeapManager = nullptr;
	std::shared_ptr<dx12::FixedSizedDescriptorHeapManager> mDsvDescriptorHeapManager = nullptr;
};

} // namespace cinder::grfx::dx12
