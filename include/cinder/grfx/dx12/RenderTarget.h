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
#include "cinder/grfx/dx12/Texture.h"
#include "cinder/grfx/RenderTarget.h"

namespace cinder::grfx::dx12 {

class CpuDescriptorHandle;

class RenderTarget;
class DepthStencil;
using RenderTargetRef = std::shared_ptr<dx12::RenderTarget>;
using DepthStencilRef = std::shared_ptr<dx12::DepthStencil>;

// ----------------------------------------------------------------------------------------------------
// RenderTarget
// ----------------------------------------------------------------------------------------------------
class RenderTarget : public dx12::DeviceChildShim<grfx::RenderTarget> {
  public:
	RenderTarget( dx12::Device *pDevice, const dx12::Texture2DRef &texture, grfx::Format format = grfx::Format::UNKNOWN, uint32_t mipLevel = 0, uint32_t arrayLayer = 0 );
	virtual ~RenderTarget() {}

	static dx12::RenderTargetRef create( dx12::Device *pDevice, const dx12::Texture2DRef &texture ) { return std::make_shared<dx12::RenderTarget>( pDevice, texture ); }

	dx12::Texture2DRef getTexture() const;

	const dx12::CpuDescriptorHandle &getDescriptorHandle() const { return mDescriptorHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE		 getD3D12DescriptorHandle() const { return mDescriptorHandle.getD3D12Handle(); }

  private:
	dx12::CpuDescriptorHandle mDescriptorHandle = {};
};

// ----------------------------------------------------------------------------------------------------
// DepthStencil
// ----------------------------------------------------------------------------------------------------
class DepthStencil : public dx12::DeviceChildShim<grfx::DepthStencil> {
  public:
	DepthStencil( dx12::Device *pDevice, const dx12::Texture2DRef &texture, grfx::Format format = grfx::Format::UNKNOWN );
	virtual ~DepthStencil() {}

	static dx12::DepthStencilRef create( dx12::Device *pDevice, const dx12::Texture2DRef &texture ) { return std::make_shared<dx12::DepthStencil>( pDevice, texture ); }

  private:
	dx12::CpuDescriptorHandle mDescriptorHandle = {};
};

} // namespace cinder::grfx::dx12
