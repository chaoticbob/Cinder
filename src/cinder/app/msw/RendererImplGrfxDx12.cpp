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

#include "cinder/app/msw/RendererImplGrfxDx12.h"
#include "cinder/app/msw/AppImplMsw.h"
#include "cinder/Log.h"

#include "cinder/grfx/dx12/Util.h"

#include <dxgidebug.h>

namespace cinder::app {

RendererImplGrfxDx12::RendererImplGrfxDx12( RendererGrfx *pRenderer )
	: RendererImplGrfx( pRenderer )
{
}

RendererImplGrfxDx12::~RendererImplGrfxDx12()
{
}

std::vector<ComPtr<IDXGIAdapter4>> RendererImplGrfxDx12::enumerateAdapters( IDXGIFactory2 *pFactory, bool includeSoftwareAdapters )
{
	std::vector<ComPtr<IDXGIAdapter4>> adapters;
	//
	{
		UINT				  adapterIndex = 0;
		ComPtr<IDXGIAdapter1> enumeratedAdapter;
		for( ; pFactory->EnumAdapters1( adapterIndex, &enumeratedAdapter ) != DXGI_ERROR_NOT_FOUND; ++adapterIndex ) {
			DXGI_ADAPTER_DESC1 desc = {};
			//
			HRESULT hr = enumeratedAdapter->GetDesc1( &desc );
			if( FAILED( hr ) ) {
				CI_LOG_W( "failed to get description for DXGI adapter " << adapterIndex << ", skipping" );
				continue;
			}

			bool isHardware = ( desc.Flags == DXGI_ADAPTER_FLAG_NONE );
			bool isSoftware = includeSoftwareAdapters && ( desc.Flags == DXGI_ADAPTER_FLAG_SOFTWARE );
			if( ! ( isHardware || isSoftware ) ) {
				CI_LOG_I( "unsupported flags for DXGI adapter " << adapterIndex << ", skipping" );
				continue;
			}

			ComPtr<IDXGIAdapter4> adapter;
			hr = enumeratedAdapter->QueryInterface( IID_PPV_ARGS( &adapter ) );
			if( FAILED( hr ) ) {
				CI_LOG_W( "failed to get required interface for DXGI adapter " << adapterIndex << ", skipping" );
				continue;
			}

			adapters.push_back( adapter );
		}
	}

	return adapters;
}

void RendererImplGrfxDx12::createDevice()
{
	const auto &options = this->getRenderer()->getOptions();

	auto adapters = enumerateAdapters( mFactory.Get() );

	for( const auto &adapter : adapters ) {
		const auto featureLevel = options.getFeatureLevel();

		ComPtr<ID3D12Device9> targetDevice;
		//
		HRESULT hr = D3D12CreateDevice( adapter.Get(), featureLevel, IID_PPV_ARGS( &targetDevice ) );
		if( SUCCEEDED( hr ) ) {
			// Check ray tracing support
			if( this->getRenderer()->getOptions().getRayTracing() ) {
				D3D12_FEATURE_DATA_D3D12_OPTIONS5 options = {};

				HRESULT hr = targetDevice->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS5, &options, sizeof( options ) );
				if( FAILED( hr ) ) {
					throw ci::Exception( "Unable to check feature support for ray tracing" );
				}

				if( options.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED ) {
					throw ci::Exception( "Ray tracing is not supported on this device" );
				}
			}

			// Check mesh shading support
			if( this->getRenderer()->getOptions().getMeshShading() ) {
				D3D12_FEATURE_DATA_D3D12_OPTIONS7 options = {};

				HRESULT hr = targetDevice->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS7, &options, sizeof( options ) );
				if( FAILED( hr ) ) {
					throw ci::Exception( "Unable to check feature support for mesh shading" );
				}

				if( options.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED ) {
					throw ci::Exception( "Mesh shading is not supported on this device" );
				}
			}
		}

		mDevice = std::make_shared<cinder::grfx::dx12::Device>(
			targetDevice,
			options.getGraphicsQueue(),
			options.getComputeQueue(),
			options.getCopyQueue() );
		break;
	}

	if( ! mDevice ) {
		throw ci::Exception( "Unable to create D3D12 device from any of the adapters on this system" );
	}
}

// void RendererImplGrfxDx12::createQueues()
//{
//	if( this->getRenderer()->getOptions().getGraphicsQueue() ) {
//		mGraphicsQueue = ci::grfx::dx12::Queue::create( mDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT );
//		mComputeQueue  = mGraphicsQueue;
//		mCopyQueue	   = mGraphicsQueue;
//	}
//
//	if( this->getRenderer()->getOptions().getComputeQueue() ) {
//		mComputeQueue = ci::grfx::dx12::Queue::create( mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE );
//		mCopyQueue	  = mComputeQueue;
//	}
//
//	if( this->getRenderer()->getOptions().getCopyQueue() ) {
//		mCopyQueue = ci::grfx::dx12::Queue::create( mDevice.Get(), D3D12_COMMAND_LIST_TYPE_COPY );
//	}
// }

void RendererImplGrfxDx12::createSwapchain()
{
	mRenderTargets.clear();

	::RECT clientRect;
	::GetClientRect( this->getRenderer()->getHwnd(), &clientRect );
	uint32_t width	= ( clientRect.right - clientRect.left );
	uint32_t height = ( clientRect.bottom - clientRect.top );

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width					= static_cast<UINT>( width );
	swapchainDesc.Height				= static_cast<UINT>( height );
	swapchainDesc.Format				= ci::grfx::dx12::toDxgiFormat( ci::grfx::Format::B8G8R8A8_UNORM );
	swapchainDesc.Stereo				= FALSE;
	swapchainDesc.SampleDesc			= { 1, 0 };
	swapchainDesc.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_SHADER_INPUT;
	swapchainDesc.BufferCount			= static_cast<UINT>( this->getRenderer()->getOptions().getSwapchainBufferCount() );
	swapchainDesc.Scaling				= DXGI_SCALING_NONE;
	swapchainDesc.SwapEffect			= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode				= DXGI_ALPHA_MODE_IGNORE;
	swapchainDesc.Flags					= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	IDXGISwapChain1 *pSwapchain = nullptr;
	//
	HRESULT hr = mFactory->CreateSwapChainForHwnd(
		mDevice->getGraphicsQueue()->getD3D12CommandQueue(),
		this->getRenderer()->getHwnd(),
		&swapchainDesc,
		nullptr, // @TODO: Add fullscreen support
		nullptr,
		&pSwapchain );
	if( FAILED( hr ) ) {
		throw ci::Exception( "Failed to create DXGI swapchain" );
	}

	hr = pSwapchain->QueryInterface( IID_PPV_ARGS( &mSwapchain ) );
	if( FAILED( hr ) ) {
		throw ci::Exception( "QueryInterface failed required DXGI swapchain version" );
	}
}

void RendererImplGrfxDx12::createSwapchainBuffers()
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	mSwapchain->GetDesc1( &desc );

	for( UINT i = 0; i < desc.BufferCount; ++i ) {
		ComPtr<ID3D12Resource> resource = nullptr;

		HRESULT hr = mSwapchain->GetBuffer( i, IID_PPV_ARGS( &resource ) );
		if( FAILED( hr ) ) {
			throw ci::Exception( "Failed to get swapchain buffer" );
		}

		auto texture = ci::grfx::dx12::Texture2D::create(
			static_cast<uint32_t>( desc.Width ),  // width
			static_cast<uint32_t>( desc.Height ), // height
			ci::grfx::Format::B8G8R8A8_UNORM,	  // format
			1,									  // sampleCount
			1,									  // mipLevelCount
			1,									  // arrayLayerCount
			resource );

		auto swapchainBuffer = ci::grfx::dx12::RenderTarget::create( texture );

		mSwapchainBuffers.push_back( swapchainBuffer );
	}
}

void RendererImplGrfxDx12::createRenderTargets()
{
	uint32_t sampleCount = this->getRenderer()->getOptions().getMsaa();
	sampleCount			 = ( sampleCount >= 2 ? sampleCount : 1 );

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	mSwapchain->GetDesc1( &desc );

	for( UINT i = 0; i < desc.BufferCount; ++i ) {
		// Render target
		{
			auto texture = ci::grfx::dx12::Texture2D::createRenderTarget(
				mDevice->getD3D12Device(),
				static_cast<uint32_t>( desc.Width ),  // width
				static_cast<uint32_t>( desc.Height ), // height
				ci::grfx::Format::B8G8R8A8_UNORM,	  // format
				sampleCount,						  // sampleCount
				1,									  // mipLevelCount
				1 );								  // arrayLayerCount

			auto renderTarget = ci::grfx::dx12::RenderTarget::create( texture );

			mRenderTargets.push_back( renderTarget );
		}

		// Depth stencil
		{
			auto texture = ci::grfx::dx12::Texture2D::createDepthStencil(
				mDevice->getD3D12Device(),
				static_cast<uint32_t>( desc.Width ),  // width
				static_cast<uint32_t>( desc.Height ), // height
				ci::grfx::Format::D32_FLOAT,		  // format
				sampleCount,						  // sampleCount
				1,									  // mipLevelCount
				1 );								  // arrayLayerCount

			auto depthStencil = ci::grfx::dx12::DepthStencil::create( texture );

			mDepthStencils.push_back( depthStencil );
		}
	}
}

void RendererImplGrfxDx12::initialize()
{
	const bool enableDebug = this->getRenderer()->getOptions().getValidation();

	if( enableDebug ) {
		// Get DXGI debug interface
		ComPtr<IDXGIDebug1> dxgiDebug;
		HRESULT				hr = DXGIGetDebugInterface1( 0, IID_PPV_ARGS( &dxgiDebug ) );
		if( FAILED( hr ) ) {
			throw ci::Exception( "DXGIGetDebugInterface1(DXGIDebug) failed" );
		}

		ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		// Get DXGI info queue
		hr = DXGIGetDebugInterface1( 0, IID_PPV_ARGS( &dxgiInfoQueue ) );
		if( FAILED( hr ) ) {
			throw ci::Exception( "DXGIGetDebugInterface1(DXGIInfoQueue) failed" );
		}

		// Set breaks
		dxgiInfoQueue->SetBreakOnSeverity( DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true );
		dxgiInfoQueue->SetBreakOnSeverity( DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true );

		// Get D3D12 debug interface
		ComPtr<ID3D12Debug> d3d12Debug;
		hr = D3D12GetDebugInterface( IID_PPV_ARGS( &d3d12Debug ) );
		if( FAILED( hr ) ) {
			throw ci::Exception( "D3D12GetDebugInterface failed" );
		}
		// Enable debug layers
		d3d12Debug->EnableDebugLayer();
	}

	// Create factory
	{
		const UINT factoryFlags = enableDebug ? DXGI_CREATE_FACTORY_DEBUG : 0;

		HRESULT hr = CreateDXGIFactory2( factoryFlags, IID_PPV_ARGS( &mFactory ) );
		if( FAILED( hr ) ) {
			throw ci::Exception( "Failed to create DXGI factory" );
		}
	}

	// Create device
	createDevice();

	// Create swapchain
	createSwapchain();

	// Create swapchain buffers and render targets
	createSwapchainBuffers();
	createRenderTargets();
}

void RendererImplGrfxDx12::kill()
{
	mDevice->waitForIdle();

	mSwapchain.Reset();
	mDevice.reset();
	mFactory.Reset();
}

void RendererImplGrfxDx12::startDraw()
{
}

void RendererImplGrfxDx12::finishDraw()
{
	this->swapBuffers();
}

void RendererImplGrfxDx12::swapBuffers()
{
	UINT bufferIndex = mSwapchain->GetCurrentBackBufferIndex();

	HRESULT hr = mSwapchain->Present( 0, 0 );
	if( FAILED( hr ) ) {
		throw ci::Exception( "Present failed for DXGI swapchain" );
	}
}

void RendererImplGrfxDx12::defaultResize()
{
	mDevice->getGraphicsQueue()->waitForIdle();

	// Release swapchain buffers and render targets
	mSwapchainBuffers.clear();
	mRenderTargets.clear();
	mDepthStencils.clear();

	::RECT clientRect;
	::GetClientRect( this->getRenderer()->getHwnd(), &clientRect );
	uint32_t width	= ( clientRect.right - clientRect.left );
	uint32_t height = ( clientRect.bottom - clientRect.top );

	HRESULT hr = mSwapchain->ResizeBuffers(
		0,							 // Use existing buffer count
		static_cast<UINT>( width ),	 //
		static_cast<UINT>( height ), //
		DXGI_FORMAT_UNKNOWN,		 // Use existing format
		DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT );
	if( FAILED( hr ) ) {
		throw ci::Exception( "Resize buffers failed for DXGI swapchain" );
	}

	// Recreate swapchain buffers and render targets
	createSwapchainBuffers();
	createRenderTargets();
}

} // namespace cinder::app
