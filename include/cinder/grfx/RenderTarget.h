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

#include "cinder/grfx/platform.h"
#include "cinder/grfx/Texture.h"

namespace cinder::grfx {

class RenderTarget;
class DepthStencil;

using RenderTargetRef = std::shared_ptr<grfx::RenderTarget>;
using DepthStencilRef = std::shared_ptr<grfx::DepthStencil>;

// ----------------------------------------------------------------------------------------------------
// RenderTarget
// ----------------------------------------------------------------------------------------------------
class RenderTarget : public grfx::DeviceChild {
  protected:
	RenderTarget( grfx::Device *pDevice, const grfx::Texture2DRef &texture, grfx::Format format, uint32_t mipLevel, uint32_t arrayLayer );

  public:
	virtual ~RenderTarget() {}

	uint32_t		   getWidth() const { return mTexture->getWidth(); }
	uint32_t		   getHeight() const { return mTexture->getHeight(); }
	grfx::Format	   getFormat() const { return mFormat; }
	uint32_t		   getMipLevel() const { return mMipLevel; }
	uint32_t		   getArrayLayer() const { return mArrayLayer; }
	grfx::Texture2DRef getTexture() const { return mTexture; }

  protected:
	grfx::Format	   mFormat	   = grfx::Format::UNKNOWN;
	uint32_t		   mMipLevel   = 0;
	uint32_t		   mArrayLayer = 0;
	grfx::Texture2DRef mTexture	   = nullptr;
};

// ----------------------------------------------------------------------------------------------------
// DepthStencil
// ----------------------------------------------------------------------------------------------------
class DepthStencil : public grfx::DeviceChild {
  protected:
	DepthStencil( grfx::Device *pDevice, const grfx::Texture2DRef &texture, grfx::Format format );

  public:
	virtual ~DepthStencil() {}

	uint32_t		   getWidth() const { return mTexture->getWidth(); }
	uint32_t		   getHeight() const { return mTexture->getHeight(); }
	grfx::Format	   getFormat() const { return mFormat; }
	grfx::Texture2DRef getTexture() const { return mTexture; }

  protected:
	grfx::Format	   mFormat	= grfx::Format::UNKNOWN;
	grfx::Texture2DRef mTexture = nullptr;
};

} // namespace cinder::grfx
