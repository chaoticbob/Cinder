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

#include "cinder/app/Renderer.h"
#include "cinder/grfx/platform.h"

#if defined( CINDER_DX12 )
#	include "cinder/grfx/dx12/platform.h"
#endif

namespace cinder::app {

typedef std::shared_ptr<class RendererGrfx> RendererGrfxRef;

class RendererImplGrfx {
  protected:
	RendererImplGrfx( RendererGrfx *pRenderer )
		: mRenderer( pRenderer ) {}

  public:
	virtual ~RendererImplGrfx() {}

  protected:
	RendererGrfx *getRenderer() const { return mRenderer; }

  private:
	friend class RendererGrfx;

	virtual void initialize() = 0;
	virtual void kill()		  = 0;

	virtual void startDraw()	 = 0;
	virtual void finishDraw()	 = 0;
	virtual void swapBuffers()	 = 0;
	virtual void defaultResize() = 0;

  private:
	RendererGrfx *mRenderer = nullptr;
};

class CI_API RendererGrfx : public Renderer {
  public:
	struct CI_API Options {
	  public:
		Options( ci::grfx::Api api = ci::grfx::Api::VULKAN )
			: mApi( api ) {}

		// clang-format off
		Options&			api( ci::grfx::Api api ) { mApi = api; return *this; }
		ci::grfx::Api		getApi() const { return mApi; }
		void				setApi( ci::grfx::Api api ) { mApi = api; }

		Options&			validation( bool enable = true ) { mValidationEnabled = enable; return *this; }
		bool				getValidation() const { return mValidationEnabled; }
		void				setValidation( bool enable = true ) { mValidationEnabled = enable; }

		Options&			graphicsQueue( bool enable = true ) { mGraphicsQueueEnabled = enable;return *this; }
		bool				getGraphicsQueue() const { return mGraphicsQueueEnabled; }
		void				setGraphicsQueue( bool enable = true ) { mGraphicsQueueEnabled = enable; }

		Options&			computeQueue( bool enable = true ) { mComputeQueueEnabled = enable;return *this; }
		bool				getComputeQueue() const { return mComputeQueueEnabled; }
		void				setComputeQueue( bool enable = true ) { mComputeQueueEnabled = enable; }

		Options&			copyQueue( bool enable = true ) { mCopyQueueEnabled = enable;return *this; }
		bool				getCopyQueue() const { return mCopyQueueEnabled; }
		void				setCopyQueue( bool enable = true ) { mCopyQueueEnabled = enable; }

#if defined( CINDER_DX12 )
		Options&			featureLevel( D3D_FEATURE_LEVEL featureLevel ) { mFeatureLevel = featureLevel; return *this; }
		D3D_FEATURE_LEVEL	getFeatureLevel() const { return mFeatureLevel; }
		void				setFeatureLevel( D3D_FEATURE_LEVEL featureLevel ) { mFeatureLevel = featureLevel; }
#endif // defined( CINDER_DX12 )

		Options&			rayTracing( bool enable = true ) { mRayTracingEnabled = enable;return *this; }
		bool				getRayTracing() const { return mRayTracingEnabled; }
		void				setRayTracing( bool enable = true ) { mRayTracingEnabled = enable; }

		Options&			meshShading( bool enable = true ) { mMeshShadingEnabled = enable;return *this; }
		bool				getMeshShading() const { return mMeshShadingEnabled; }
		void				setMeshShading( bool enable = true ) { mMeshShadingEnabled = enable; }
		// clang-format on

	  private:
		ci::grfx::Api mApi					= ci::grfx::Api::VULKAN;
		bool		  mValidationEnabled	= false;
		bool		  mGraphicsQueueEnabled = true;
		bool		  mComputeQueueEnabled	= false;
		bool		  mCopyQueueEnabled		= false;

#if defined( CINDER_DX12 )
		D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_12_2;
#endif // defined( CINDER_DX12 )

		bool mRayTracingEnabled	 = false;
		bool mMeshShadingEnabled = false;
	};

	RendererGrfx( const Options &options );
	virtual ~RendererGrfx();

	virtual RendererRef clone() const;

	const Options &getOptions() const { return mOptions; }

#if defined( CINDER_MSW_DESKTOP )
	virtual void setup( WindowImplMsw *windowImpl, RendererRef sharedRenderer ) override;
	virtual void kill() override;
	virtual HWND getHwnd() const override;
	virtual HDC	 getDc() const override;
#endif

	virtual Surface8u copyWindowSurface( const Area &area, int32_t windowHeightPixels ) override { return Surface8u(); }

	virtual void startDraw() override;
	virtual void finishDraw() override;
	virtual void swapBuffers() override;
	virtual void defaultResize() override;

  private:
	RendererGrfx( const RendererGrfx &renderer );

  private:
	Options mOptions = {};

#if defined( CINDER_MSW_DESKTOP )
	WindowImplMsw *mWindowImpl = nullptr;
#endif

	std::unique_ptr<RendererImplGrfx> mImpl = nullptr;
};

} // namespace cinder::app
