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

#include "cinder/Exception.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include <wrl/client.h>

#include <memory>

namespace cinder::grfx::dx12 {

using Microsoft::WRL::ComPtr;

class DescriptorHeap;
class Device;

// ----------------------------------------------------------------------------------------------------
// DeviceChildShim
// ----------------------------------------------------------------------------------------------------
template <typename BaseT>
class DeviceChildShim : public BaseT {
  public:
	template <typename... ArgsT>
	DeviceChildShim( ArgsT &&...args )
		: BaseT( std::forward<ArgsT>( args )... ) {}

	virtual ~DeviceChildShim() {}

	dx12::Device *getDevice() const { return static_cast<dx12::Device *>( BaseT::getDevice() ); }
};

// ----------------------------------------------------------------------------------------------------
// CpuDescriptorHandle
// ----------------------------------------------------------------------------------------------------
class CpuDescriptorHandle {
  public:
	CpuDescriptorHandle() {}

	CpuDescriptorHandle( const dx12::DescriptorHeap *pHeap, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle )
		: mHeap( pHeap ), mDescriptorHandle( descriptorHandle ) {}

	~CpuDescriptorHandle() {}

	const dx12::DescriptorHeap		  *getHeap() const { return mHeap; }
	const D3D12_CPU_DESCRIPTOR_HANDLE &getD3D12Handle() const { return mDescriptorHandle; }

  private:
	const dx12::DescriptorHeap *mHeap			  = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE mDescriptorHandle = {};
};

} // namespace cinder::grfx::dx12
