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
#include "cinder/grfx/Texture.h"

namespace cinder::grfx::dx12 {

class Texture1D;
class Texture2D;
class Texture3D;
using Texture1DRef = std::shared_ptr<dx12::Texture1D>;
using Texture2DRef = std::shared_ptr<dx12::Texture2D>;
using Texture3DRef = std::shared_ptr<dx12::Texture3D>;

// ----------------------------------------------------------------------------------------------------
// TextureBase
// ----------------------------------------------------------------------------------------------------
template <typename BaseT>
class TextureShim : public dx12::DeviceChildShim<BaseT> {
  public:
	TextureShim( dx12::Device* pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount )
		: dx12::DeviceChildShim<BaseT>( pDevice, width, height, format, sampleCount ) {}

	TextureShim( dx12::Device* pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount, const ComPtr<ID3D12Resource> &resource )
		: dx12::DeviceChildShim<BaseT>( pDevice, width, height, format, sampleCount, mipLevelCount, arrayLayerCount ),
		  mResource( resource ) {}

	virtual ~TextureShim() {}

	ID3D12Resource *getD3D12Resource() const { return mResource.Get(); }

  protected:
	ComPtr<ID3D12Resource> mResource;
};

// ----------------------------------------------------------------------------------------------------
// Texture1D
// ----------------------------------------------------------------------------------------------------
class Texture1D : public  dx12::TextureShim<grfx::Texture1D> {
  public:
	virtual ~Texture1D() {}
};

// ----------------------------------------------------------------------------------------------------
// Texture2D
// ----------------------------------------------------------------------------------------------------
class Texture2D : public dx12::TextureShim<grfx::Texture2D> {
  public:
	Texture2D( dx12::Device* pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount, const ComPtr<ID3D12Resource> &resource )
		: dx12::TextureShim<grfx::Texture2D>( pDevice, width, height, format, sampleCount, mipLevelCount, arrayLayerCount, resource ) {}

	virtual ~Texture2D() {}

	static dx12::Texture2DRef create( dx12::Device* pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount, const ComPtr<ID3D12Resource> &resource );
	static dx12::Texture2DRef create( dx12::Device* pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount, bool writeable );
	static dx12::Texture2DRef createRenderTarget( dx12::Device* pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount );
	static dx12::Texture2DRef createDepthStencil( dx12::Device* pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount );
};

// ----------------------------------------------------------------------------------------------------
// Texture3D
// ----------------------------------------------------------------------------------------------------
class Texture3D : public  dx12::TextureShim<grfx::Texture3D> {
  public:
	virtual ~Texture3D() {}
};

} // namespace cinder::grfx::dx12
