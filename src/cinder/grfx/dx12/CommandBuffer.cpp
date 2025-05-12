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
#include "cinder/grfx/dx12/RenderTarget.h"
#include "cinder/grfx/dx12/Util.h"

namespace cinder::grfx::dx12 {

// ----------------------------------------------------------------------------------------------------
// CommandBuffer
// ----------------------------------------------------------------------------------------------------
CommandBuffer::CommandBuffer( dx12::Queue *pParentQueue )
	: dx12::DeviceChildShim<grfx::CommandBuffer>( pParentQueue )
{
	HRESULT hr = getD3D12Device()->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &mCommandAllocator ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 command allocator" );
	}

	hr = this->getD3D12Device()->CreateCommandList1( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS( &mCommandList ) );
	if( FAILED( hr ) ) {
		throw cinder::Exception( "Failed to create D3D12 command list" );
	}
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::submit()
{
}

void CommandBuffer::flush()
{
}

void CommandBuffer::reset()
{
	mCommandList->Reset( mCommandAllocator.Get(), nullptr );
}

void CommandBuffer::close()
{
	mCommandList->Close();
}

void CommandBuffer::beginRenderPass( const grfx::RenderPass &renderPass )
{
	std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> renderTargets = {};
	//
	for( const auto &attachment : renderPass.getColorAttachments() ) {
		auto srcRenderTarget = std::static_pointer_cast<dx12::RenderTarget>( attachment.renderTarget() );
		auto rtvDescriptor	 = srcRenderTarget->getDescriptorHandle().getD3D12Handle();

		D3D12_RENDER_PASS_RENDER_TARGET_DESC desc = {};
		desc.cpuDescriptor						  = rtvDescriptor;

		switch( attachment.beginOp() ) {
			default: {
				throw cinder::Exception( "Unsupported attachment begin op" );
			} break;

			case grfx::BeginOp::CLEAR: {
				desc.BeginningAccess.Type					   = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
				desc.BeginningAccess.Clear.ClearValue.Format   = toDxgiFormat( srcRenderTarget->getFormat() );
				desc.BeginningAccess.Clear.ClearValue.Color[0] = attachment.clearColor().r;
				desc.BeginningAccess.Clear.ClearValue.Color[1] = attachment.clearColor().g;
				desc.BeginningAccess.Clear.ClearValue.Color[2] = attachment.clearColor().b;
				desc.BeginningAccess.Clear.ClearValue.Color[3] = attachment.clearColor().a;
			} break;

			case grfx::BeginOp::LOAD: desc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE; break;
			case grfx::BeginOp::OVERWRITE: desc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD; break;
		}

		switch( attachment.endOp() ) {
			default: {
				throw cinder::Exception( "Unsupported attachment end op" );
			} break;

			case grfx::EndOp::STORE: desc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE; break;
			case grfx::EndOp::DISCARD: desc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD; break;
		}

		renderTargets.push_back( desc );
	}

	D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_NONE;

	mCommandList->BeginRenderPass(
		static_cast<UINT>( renderTargets.size() ),
		renderTargets.empty() ? nullptr : renderTargets.data(),
		nullptr,
		flags );
}

void CommandBuffer::endRenderPass()
{
	mCommandList->EndRenderPass();
}

void CommandBuffer::resolveSubresource( const grfx::Texture2D *pDstTexture, uint32_t dstSubResourceIndex, const grfx::Texture2D *pSrcTexture, uint32_t srcSubResourceIndex )
{
}

} // namespace cinder::grfx::dx12

