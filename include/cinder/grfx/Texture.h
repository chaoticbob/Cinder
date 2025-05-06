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

using Texture1DRef = std::shared_ptr<class Texture1D>;
using Texture2DRef = std::shared_ptr<class Texture2D>;
using Texture3DRef = std::shared_ptr<class Texture3D>;

// ----------------------------------------------------------------------------------------------------
// TextureBase
// ----------------------------------------------------------------------------------------------------
class TextureBase {
  public:
	TextureBase() {}
	virtual ~TextureBase() {}

	virtual uint32_t getWidth() const  = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getDepth() const  = 0;

	grfx::Format getFormat() const { return mFormat; }
	uint32_t	 getSamples() const { return mSamples; }

  protected:
	grfx::Format mFormat  = grfx::Format::UNKNOWN;
	uint32_t	 mSamples = 1;
};

// ----------------------------------------------------------------------------------------------------
// Texture1D
// ----------------------------------------------------------------------------------------------------
class Texture1D : public TextureBase {
  public:
	Texture1D() {}
	virtual ~Texture1D() {}

	virtual uint32_t getWidth() const { return mWidth; }
	virtual uint32_t getHeight() const { return 1; }
	virtual uint32_t getDepth() const { return 1; }

  protected:
	uint32_t mWidth = 0;
};

// ----------------------------------------------------------------------------------------------------
// Texture2D
// ----------------------------------------------------------------------------------------------------
class Texture2D : public TextureBase {
  public:
	Texture2D() {}
	virtual ~Texture2D() {}

	virtual uint32_t getWidth() const { return mWidth; }
	virtual uint32_t getHeight() const { return mHeight; }
	virtual uint32_t getDepth() const { return 1; }

  protected:
	uint32_t mWidth	 = 0;
	uint32_t mHeight = 0;
};

// ----------------------------------------------------------------------------------------------------
// Texture3D
// ----------------------------------------------------------------------------------------------------
class Texture3D : public TextureBase {
  public:
	Texture3D() {}
	virtual ~Texture3D() {}

	virtual uint32_t getWidth() const { return mWidth; }
	virtual uint32_t getHeight() const { return mHeight; }
	virtual uint32_t getDepth() const { return mDepth; }

  protected:
	uint32_t mWidth	 = 0;
	uint32_t mHeight = 0;
	uint32_t mDepth	 = 0;
};

} // namespace cinder::grfx
