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

#include "cinder/grfx/dx12/Texture.h"
#include "cinder/grfx/dx12/Util.h"

namespace cinder::grfx::dx12 {

//// ----------------------------------------------------------------------------------------------------
//// TextureResource
//// ----------------------------------------------------------------------------------------------------
// TextureResource::TextureResource( uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount )
//{
//	D3D12_RESOURCE_DESC desc = {};
//	desc.Dimension			 = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	desc.Alignment			 = 0;
//	desc.Width				 = static_cast<UINT64>( width );
//	desc.Height				 = static_cast<UINT>( height );
//	desc.DepthOrArraySize	 = numArrayLayers;
//	desc.MipLevels			 = numMipLevels;
//	desc.Format				 = format;
//	desc.SampleDesc			 = { 1, 0 };
//	desc.Layout				 = D3D12_TEXTURE_LAYOUT_UNKNOWN;
//	desc.Flags				 = D3D12_RESOURCE_FLAG_NONE;
//
//	D3D12_HEAP_PROPERTIES heapProperties = {};
//	heapProperties.Type					 = D3D12_HEAP_TYPE_DEFAULT;
//
//	HRESULT hr = pRenderer->Device->CreateCommittedResource(
//		&heapProperties,				// pHeapProperties
//		D3D12_HEAP_FLAG_NONE,			// HeapFlags
//		&desc,							// pDesc
//		D3D12_RESOURCE_STATE_COPY_DEST, // InitialResourceState
//		nullptr,						// pOptimizedClearValues
//		IID_PPV_ARGS( ppResource ) );	// riidResource, ppvResouce
//	if( FAILED( hr ) ) {
//		return hr;
//	}
// }

static ComPtr<ID3D12Resource> createTextureResource(
	ID3D12Device		 *pDevice,
	uint32_t			  width,
	uint32_t			  height,
	DXGI_FORMAT			  format,
	uint32_t			  sampleCount,
	uint32_t			  mipLevelCount,
	uint32_t			  arrayLayerCount,
	D3D12_RESOURCE_FLAGS  resourceFlags,
	D3D12_RESOURCE_STATES initialResourceState )
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension			 = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment			 = 0;
	desc.Width				 = static_cast<UINT64>( width );
	desc.Height				 = static_cast<UINT>( height );
	desc.DepthOrArraySize	 = static_cast<UINT16>( arrayLayerCount );
	desc.MipLevels			 = static_cast<UINT16>( mipLevelCount );
	desc.Format				 = format;
	desc.SampleDesc			 = { static_cast<UINT>( sampleCount ), 0 };
	desc.Layout				 = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags				 = resourceFlags;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type					 = D3D12_HEAP_TYPE_DEFAULT;

	ComPtr<ID3D12Resource> resource = nullptr;
	//
	HRESULT hr = pDevice->CreateCommittedResource(
		&heapProperties,			 // pHeapProperties
		D3D12_HEAP_FLAG_NONE,		 // HeapFlags
		&desc,						 // pDesc
		initialResourceState,		 // InitialResourceState
		nullptr,					 // pOptimizedClearValues
		IID_PPV_ARGS( &resource ) ); // riidResource, ppvResouce
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 texture resource" );
	}

	return resource;
}

// ----------------------------------------------------------------------------------------------------
// Texture2D
// ----------------------------------------------------------------------------------------------------
dx12::Texture2DRef Texture2D::create( uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount, const ComPtr<ID3D12Resource> &resource )
{
	return std::make_shared<dx12::Texture2D>(
		width,
		height,
		format,
		sampleCount,
		mipLevelCount,
		arrayLayerCount,
		resource );
}

dx12::Texture2DRef Texture2D::create( ID3D12Device *pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount, bool writeable )
{
	auto resource = createTextureResource(
		pDevice,
		width,
		height,
		toDxgiFormat( format ),
		sampleCount,
		mipLevelCount,
		arrayLayerCount,
		writeable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE,
		writeable ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COPY_DEST );

	return std::make_shared<dx12::Texture2D>( width, height, format, sampleCount, mipLevelCount, arrayLayerCount, resource );
}

dx12::Texture2DRef Texture2D::createRenderTarget( ID3D12Device *pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount )
{
	auto resource = createTextureResource(
		pDevice,
		width,
		height,
		toDxgiFormat( format ),
		sampleCount,
		mipLevelCount,
		arrayLayerCount,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RENDER_TARGET );

	return std::make_shared<dx12::Texture2D>( width, height, format, sampleCount, mipLevelCount, arrayLayerCount, resource );
}

dx12::Texture2DRef Texture2D::createDepthStencil( ID3D12Device *pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount )
{
	auto resource = createTextureResource(
		pDevice,
		width,
		height,
		toDxgiFormat( format ),
		sampleCount,
		mipLevelCount,
		arrayLayerCount,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_RESOURCE_STATE_DEPTH_WRITE );

	return std::make_shared<dx12::Texture2D>( width, height, format, sampleCount, mipLevelCount, arrayLayerCount, resource );
}

} // namespace cinder::grfx::dx12
