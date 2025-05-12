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

#include <string>

namespace cinder::grfx {

class ShaderModule;
class ProgramParam;
class ProgramSig;
class Program;

using ShaderModuleRef = std::shared_ptr<grfx::ShaderModule>;
using ProgramParamRef = std::shared_ptr<grfx::ProgramParam>;
using ProgramSigRef	  = std::shared_ptr<grfx::ProgramSig>;
using ProgramRef	  = std::shared_ptr<grfx::Program>;

enum class SourceLanguage
{
	SLANG = 0,
	GLSL  = 1,
	HLSL  = 2,
	METAL = 3,
};

// ----------------------------------------------------------------------------------------------------
// ProgramParam
// ----------------------------------------------------------------------------------------------------
class ProgramParam {
  public:
	enum class Type
	{
		UNKNOWN				   = 0,
		ROOT_CONSTANT		   = 1,
		CONSTANT_BUFFER		   = 2,
		STRUCTURED_BUFFER	   = 3,
		STORAGE_BUFFER		   = 4,
		TEXTURE				   = 5,
		STORAGE_TEXTURE		   = 6,
		ACCELERATION_STRUCTURE = 7,
	};

	ProgramParam( const std::string &aName, Type aType, uint32_t aArraySize = 1 )
		: mName( aName ), mType( aType ), mArraySize( aArraySize ) {}

	virtual ~ProgramParam() {}

	const std::string &name() const { return mName; }
	Type			   type() const { return mType; }
	uint32_t		   arraySize() const { return mArraySize; }

  private:
	std::string mName	   = "";
	Type		mType	   = Type::UNKNOWN;
	uint32_t	mArraySize = 1;
};

// ----------------------------------------------------------------------------------------------------
// ProgramSig
// ----------------------------------------------------------------------------------------------------
class ProgramSig {
  public:
	ProgramSig( const std::vector<grfx::ProgramParamRef> &aParams )
		: mParams( aParams ) {}

	virtual ~ProgramSig() {}

	const std::vector<grfx::ProgramParamRef> &params() const { return mParams; }

  private:
	std::vector<grfx::ProgramParamRef> mParams;
};

// ----------------------------------------------------------------------------------------------------
// Program
// ----------------------------------------------------------------------------------------------------
class Program {
  public:
	Program();
	virtual ~Program();

	const grfx::ProgramSig *sig() const { return mSig.get(); }

  protected:
	grfx::ProgramSigRef mSig = nullptr;
};

// ----------------------------------------------------------------------------------------------------
// ShaderModule
// ----------------------------------------------------------------------------------------------------
class ShaderModule {
  public:
};

// ----------------------------------------------------------------------------------------------------
// Compiler
// ----------------------------------------------------------------------------------------------------
class Compiler {
  public:
	Compiler() {}
	virtual ~Compiler() {}

	virtual bool				  supportsSourceLanguage( grfx::SourceLanguage language ) const = 0;
	virtual grfx::ShaderModuleRef compile(grfx::SourceLanguage sourceLanguage);
};

// ----------------------------------------------------------------------------------------------------
// Compile functions
// ----------------------------------------------------------------------------------------------------
grfx::ProgramRef createGraphicsProgramFromSource(
	grfx::SourceLanguage sourceLanguage,
	const std::string	&vertexSource,
	const std::string	&vertexEntryPoint,
	const std::string	&pixelSource,
	const std::string	&pixelEntryPoint );

grfx::ProgramRef createComputeProgramFromSource(
	grfx::SourceLanguage sourceLanguage,
	const std::string	&source,
	const std::string	&entryPoint );

} // namespace cinder::grfx
