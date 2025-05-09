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

namespace cinder::grfx {

class Texture1D;
class Texture2D;
class Texture3D;
using Texture1DRef = std::shared_ptr<grfx::Texture1D>;
using Texture2DRef = std::shared_ptr<grfx::Texture2D>;
using Texture3DRef = std::shared_ptr<grfx::Texture3D>;

// ----------------------------------------------------------------------------------------------------
// TextureBase
// ----------------------------------------------------------------------------------------------------
class TextureBase : public grfx::DeviceChild {
  public:
	TextureBase( grfx::Device *pDevice, uint32_t width, uint32_t height, uint32_t depth, grfx::Format format, uint32_t mipLevelCount, uint32_t arrayLayerCount )
		: grfx::DeviceChild( pDevice ), mWidth( width ), mHeight( height ), mDepth( depth ), mFormat( format ), mMipLevelCount( mipLevelCount ), mArrayLayerCount( arrayLayerCount ) {}

	virtual ~TextureBase() {}

	uint32_t	 getWidth() const { return mWidth; }
	uint32_t	 getHeight() const { return mHeight; }
	uint32_t	 getDepth() const { return mDepth; }
	grfx::Format getFormat() const { return mFormat; }
	uint32_t	 getMipLevelCount() const { return mMipLevelCount; }
	uint32_t	 getArrayLayerCount() const { return mArrayLayerCount; }

  protected:
	uint32_t	 mWidth			  = 0;
	uint32_t	 mHeight		  = 0;
	uint32_t	 mDepth			  = 0;
	grfx::Format mFormat		  = grfx::Format::UNKNOWN;
	uint32_t	 mMipLevelCount	  = 0;
	uint32_t	 mArrayLayerCount = 0;
};

// ----------------------------------------------------------------------------------------------------
// Texture1D
// ----------------------------------------------------------------------------------------------------
class Texture1D : public grfx::TextureBase {
  public:
	virtual ~Texture1D() {}
};

// ----------------------------------------------------------------------------------------------------
// Texture2D
// ----------------------------------------------------------------------------------------------------
class Texture2D : public grfx::TextureBase {
  public:
	Texture2D( grfx::Device *pDevice, uint32_t width, uint32_t height, grfx::Format format, uint32_t sampleCount, uint32_t mipLevelCount, uint32_t arrayLayerCount )
		: grfx::TextureBase( pDevice, width, height, 1, format, mipLevelCount, arrayLayerCount ),
		  mSampleCount( std::max<uint32_t>( sampleCount, 1 ) ) {}

	virtual ~Texture2D() {}

	uint32_t getSampleCount() const { return mSampleCount; }

  protected:
	uint32_t mSampleCount = 1;
};

// ----------------------------------------------------------------------------------------------------
// Texture3D
// ----------------------------------------------------------------------------------------------------
class Texture3D : public grfx::TextureBase {
  public:
	virtual ~Texture3D() {}
};

} // namespace cinder::grfx
