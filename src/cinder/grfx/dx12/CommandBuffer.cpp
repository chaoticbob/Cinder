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

#include "cinder/grfx/dx12/CommandBuffer.h"
#include "cinder/grfx/dx12/Device.h"
#include "cinder/grfx/dx12/Queue.h"

namespace cinder::grfx::dx12 {

// ----------------------------------------------------------------------------------------------------
// CommandBufferBaseImpl
// ----------------------------------------------------------------------------------------------------
CommandBufferBaseImpl::CommandBufferBaseImpl( dx12::Queue *pParentQueue )
{
	D3D12_COMMAND_LIST_TYPE commandType = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if( pParentQueue->getCommandType() == cinder::grfx::CommandType::COMPUTE ) {
		commandType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	}
	else if( pParentQueue->getCommandType() == cinder::grfx::CommandType::COPY ) {
		commandType = D3D12_COMMAND_LIST_TYPE_COPY;
	}

	HRESULT hr = pParentQueue->getDevice()->getD3D12Device()->CreateCommandAllocator( commandType, IID_PPV_ARGS( &mCommandAllocator ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 command allocator" );
	}

	hr = pParentQueue->getDevice()->getD3D12Device()->CreateCommandList( 0, commandType, nullptr, nullptr, IID_PPV_ARGS( &mCommandList ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 command list" );
	}
}

CommandBufferBaseImpl::~CommandBufferBaseImpl()
{
	mCommandList.Reset();
	mCommandAllocator.Reset();
}

void CommandBufferBaseImpl::submitCommands()
{
}

void CommandBufferBaseImpl::flushCommands()
{
}

// ----------------------------------------------------------------------------------------------------
// GraphicsCommandBuffer
// ----------------------------------------------------------------------------------------------------
void GraphicsCommandBuffer::Reset()
{
	this->getD3D12CommandList()->Reset( this->getD3D12CommandAllocator(), nullptr );
}

void GraphicsCommandBuffer::Close()
{
	this->getD3D12CommandList()->Close();
}

void GraphicsCommandBuffer::BeginRendering( const std::vector<grfx::RenderTargetRef> &renderTargets, grfx::DepthStencilRef &depthStencil )
{
	std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> renderTargetDescs = {};
	for( const auto &renderTarget : renderTargets ) {
		D3D12_RENDER_PASS_RENDER_TARGET_DESC desc = {};
	}

	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencilDesc = {};
	if( depthStencil ) {
	}

	D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_NONE;

	this->getD3D12CommandList()->BeginRenderPass(
		static_cast<UINT>( renderTargetDescs.size() ),
		renderTargetDescs.empty() ? nullptr : renderTargetDescs.data(),
		depthStencil ? &depthStencilDesc : nullptr,
		flags );
}

void GraphicsCommandBuffer::EndRendering()
{
	this->getD3D12CommandList()->EndRenderPass();
}

void GraphicsCommandBuffer::ClearRenderTarget( uint32_t renderTargetIndex, float r, float g, float b, float a )
{
}

void GraphicsCommandBuffer::ResolveSubresource( const grfx::Texture2D *pDstTexture, uint32_t dstSubResourceIndex, const grfx::Texture2D *pSrcTexture, uint32_t srcSubResourceIndex )
{
}

} // namespace cinder::grfx::dx12

