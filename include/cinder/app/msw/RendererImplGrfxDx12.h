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

#include "cinder/app/RendererGrfx.h"
#include "cinder/grfx/dx12/Queue.h"
#include "cinder/grfx/dx12/RenderTarget.h"

namespace cinder::app {

using cinder::grfx::dx12::ComPtr;

class RendererGrfx;

class RendererImplGrfxDx12 : public RendererImplGrfx {
  public:
	RendererImplGrfxDx12( RendererGrfx *pRenderer );
	virtual ~RendererImplGrfxDx12();

  private:
	static std::vector<ComPtr<IDXGIAdapter4>> enumerateAdapters( IDXGIFactory2 *pFactory, bool includeSoftwareAdapters = false );

	void createDevice();
	void createQueues();
	void createSwapchain();
	void createSwapchainBuffers();
	void createRenderTargets();

	virtual void initialize() override;
	virtual void kill() override;

	void waitForIdle();

	virtual void startDraw() override;
	virtual void finishDraw() override;
	virtual void swapBuffers() override;
	virtual void defaultResize() override;

  private:
	ComPtr<IDXGIFactory2>							 mFactory		   = nullptr;
	ComPtr<ID3D12Device9>							 mDevice		   = nullptr;
	cinder::grfx::dx12::QueueRef					 mGraphicsQueue	   = nullptr;
	cinder::grfx::dx12::QueueRef					 mComputeQueue	   = nullptr;
	cinder::grfx::dx12::QueueRef					 mCopyQueue		   = nullptr;
	ComPtr<IDXGISwapChain4>							 mSwapchain		   = nullptr;
	std::vector<cinder::grfx::dx12::RenderTargetRef> mSwapchainBuffers = {};
	std::vector<cinder::grfx::dx12::RenderTargetRef> mRenderTargets	   = {};
	std::vector<cinder::grfx::dx12::DepthStencilRef> mDepthStencils	   = {};
};

} // namespace cinder::app
