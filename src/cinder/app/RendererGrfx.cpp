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

#include "cinder/app/RendererGrfx.h"
#include "cinder/app/msw/AppImplMsw.h"
#include "cinder/Log.h"

#if defined( CINDER_DX12 )
#	include "cinder/app/msw/RendererImplGrfxDx12.h"
#endif

namespace cinder::app {

RendererGrfx::RendererGrfx( const Options &options )
	: Renderer(), mImpl( nullptr ), mOptions( options )
{
}

RendererGrfx::RendererGrfx( const RendererGrfx &renderer )
	: Renderer( renderer ), mImpl( nullptr ), mOptions( renderer.mOptions )
{
}

RendererGrfx::~RendererGrfx()
{
}

RendererRef RendererGrfx::clone() const
{
	return RendererGrfxRef( new RendererGrfx( *this ) );
}

#if defined( CINDER_MSW_DESKTOP )
void RendererGrfx::setup( WindowImplMsw *windowImpl, RendererRef sharedRenderer )
{
	mWindowImpl = windowImpl;

	switch( mOptions.getApi() ) {
#	if defined( CINDER_DX12 )
		case ci::grfx::Api::DX12: {
			mImpl = std::make_unique<RendererImplGrfxDx12>( this );
		} break;
#	endif
	}

	if( !mImpl ) {
		throw ci::Exception( "Unsupported graphics API" );
	}

	mImpl->initialize();
}

void RendererGrfx::kill()
{
}

HWND RendererGrfx::getHwnd() const
{
	return mWindowImpl->getHwnd();
}

HDC RendererGrfx::getDc() const
{
	return mWindowImpl->getDc();
}
#endif // defined( CINDER_MSW_DESKTOP )

void RendererGrfx::startDraw()
{
	mImpl->startDraw();
}

void RendererGrfx::finishDraw()
{
	mImpl->finishDraw();
}

void RendererGrfx::swapBuffers()
{
	mImpl->swapBuffers();
}

void RendererGrfx::defaultResize()
{
	mImpl->defaultResize();
}

} // namespace cinder::app
