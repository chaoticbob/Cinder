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

#include "cinder/grfx/dx12/RenderTarget.h"
#include "cinder/grfx/dx12/Device.h"
#include "cinder/grfx/dx12/Util.h"

namespace cinder::grfx::dx12 {

// ----------------------------------------------------------------------------------------------------
// RenderTarget
// ----------------------------------------------------------------------------------------------------
RenderTarget::RenderTarget( dx12::Device *pDevice, const dx12::Texture2DRef &texture, grfx::Format format, uint32_t mipLevel, uint32_t arrayLayer )
	: dx12::DeviceChildShim<grfx::RenderTarget>( pDevice, texture, format, mipLevel, arrayLayer ),
	  mDescriptorHandle( texture->getDevice()->allocateHandle( D3D12_DESCRIPTOR_HEAP_TYPE_RTV ) )
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format						   = toDxgiFormat( this->getFormat() );
	desc.ViewDimension				   = D3D12_RTV_DIMENSION_TEXTURE2D;

	// Multisample textures can only have 1 mip level and 1 array layer
	const bool isMultisSample = ( texture->getSampleCount() > 1 );
	if( ! isMultisSample ) {
		desc.Texture2D.MipSlice	  = this->getMipLevel();
		desc.Texture2D.PlaneSlice = this->getArrayLayer();
	}

	ID3D12Resource *pResource = this->getTexture()->getD3D12Resource();
	this->getD3D12Device()->CreateRenderTargetView(
		pResource,
		&desc,
		mDescriptorHandle.getD3D12Handle() );
}

dx12::Texture2DRef RenderTarget::getTexture() const
{
	return std::static_pointer_cast<dx12::Texture2D>( dx12::DeviceChildShim<grfx::RenderTarget>::getTexture() );
}

// ----------------------------------------------------------------------------------------------------
// DepthStencil
// ----------------------------------------------------------------------------------------------------
DepthStencil::DepthStencil( dx12::Device *pDevice, const dx12::Texture2DRef &texture, grfx::Format format )
	: dx12::DeviceChildShim<grfx::DepthStencil>( pDevice, texture, format ),
	  mDescriptorHandle( texture->getDevice()->allocateHandle( D3D12_DESCRIPTOR_HEAP_TYPE_DSV ) )
{
}

} // namespace cinder::grfx::dx12
