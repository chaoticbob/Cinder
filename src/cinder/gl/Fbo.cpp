/*
 Copyright (c) 2013, The Cinder Project
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org

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

// Relevant OpenGL Extensions
// * OES_packed_depth_stencil http://www.khronos.org/registry/gles/extensions/OES/OES_packed_depth_stencil.txt
//	 * DEPTH_STENCIL_OES - <format> param of Tex(Sub)Image2D, the <internalformat> param of TexImage2D
//	 * UNSIGNED_INT_24_8_OES - <type> param of Tex(Sub)Image2D
//	 * DEPTH24_STENCIL8_OES - <internalformat> param of RenderbufferStorage
// * EXT_packed_depth_stencil http://www.opengl.org/registry/specs/EXT/packed_depth_stencil.txt
// * http://www.khronos.org/registry/gles/extensions/ANGLE/ANGLE_framebuffer_multisample.txt

// Both ANGLE and iOS support OES_depth_texture (ANGLE_depth_texture) so we support it everywhere

#include "cinder/gl/platform.h" // must be first
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Environment.h"
#include "cinder/gl/scoped.h"
#include "cinder/Log.h"
#include "cinder/Camera.h"
#include "cinder/gl/ConstantConversions.h"
#include "cinder/ip/Flip.h"

using namespace std;

#if ! defined( CINDER_GL_ES_2 )
	#define MAX_COLOR_ATTACHMENT	GL_COLOR_ATTACHMENT15
#else
	#define MAX_COLOR_ATTACHMENT	GL_COLOR_ATTACHMENT0
#endif

namespace cinder {
namespace gl {

std::map<GLenum, GLint> Fbo::sNumSampleCounts;
GLint Fbo::sMaxSamples = -1;
GLint Fbo::sMaxAttachments = -1;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Support functions
GLenum determineAspectFromFormat( GLenum internalFormat )
{
	GLenum result = ( GL_INVALID_ENUM != internalFormat ) ?  GL_COLOR : GL_INVALID_ENUM;
	switch( internalFormat ) {
#if defined( CINDER_GL_ES_2 )
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
			result = GL_DEPTH;	
		break;

		case GL_DEPTH24_STENCIL8:
			result = GL_DEPTH_STENCIL;	
		break;
#else
	    case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
			result = GL_DEPTH;
		break;

		case GL_DEPTH_STENCIL:
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
			result = GL_DEPTH_STENCIL;
		break;
#endif
#if ! defined( CINDER_GL_ANGLE )
		case GL_STENCIL_INDEX:
#endif
		case GL_STENCIL_INDEX8:
			result = GL_STENCIL;
		break;
	}
	return result;
}

GLenum determineAspectFromAttachmentPoint( GLenum attachmentPoint )
{
	GLenum result = GL_INVALID_ENUM;
	switch( attachmentPoint ) {
		case GL_DEPTH_ATTACHMENT: {
			result = GL_DEPTH;
		}
		break;

		case GL_STENCIL_ATTACHMENT: {
			result = GL_STENCIL;
		}
		break;

		case GL_DEPTH_STENCIL_ATTACHMENT: {
			result = GL_DEPTH_STENCIL;
		}
		break;

		default: {
			if( (  attachmentPoint >= GL_COLOR_ATTACHMENT0 ) && ( attachmentPoint < ( GL_COLOR_ATTACHMENT0 + Fbo::getMaxAttachments() ) ) ) {
				result = GL_COLOR;
			}
		}
		break;
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Renderbuffer
RenderbufferRef Renderbuffer::create( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples )
{
	return RenderbufferRef( new Renderbuffer( width, height, internalFormat, msaaSamples, coverageSamples ) );
}

Renderbuffer::Renderbuffer( int width, int height, GLenum internalFormat, int msaaSamples, int coverageSamples )
{
	mWidth = width;
	mHeight = height;
	mInternalFormat = internalFormat;
	mSamples = msaaSamples;
	mCoverageSamples = coverageSamples;

	static bool csaaSupported = gl::env()->supportsCoverageSample();

	glGenRenderbuffers( 1, &mId );

	if( mSamples > Fbo::getMaxSamples() )
		mSamples = Fbo::getMaxSamples();

	if( ! csaaSupported ) {
		mCoverageSamples = 0;
	}

	gl::ScopedRenderbuffer rbb( GL_RENDERBUFFER, mId );

	if( gl::env()->supportsFboMultiSample() ) {
		// create a CSAA buffer
		if( mCoverageSamples ) {
#if defined( CINDER_GL_ES )
			// @TODO: Add coverage sampling support
#else
			glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER, mCoverageSamples, mSamples, mInternalFormat, mWidth, mHeight );
#endif
		}
		else {
			if( mSamples ) {
				glRenderbufferStorageMultisample( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
			}
			else {
				glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );		
			}
		}
	}
	else {
#if defined( CINDER_GL_ES_2 )
		// This is gross, but GL_RGBA & GL_RGB are not suitable internal formats for Renderbuffers. We know what you meant though.
		if( mInternalFormat == GL_RGBA ) {
			mInternalFormat = GL_RGBA8_OES;
		}
		else if( mInternalFormat == GL_RGB ) {
			mInternalFormat = GL_RGB8_OES;
		}
		else if( mInternalFormat == GL_DEPTH_COMPONENT ) {
			mInternalFormat = GL_DEPTH_COMPONENT24_OES;	
		}
#endif
		glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );
	}
}

Renderbuffer::~Renderbuffer()
{
	auto ctx = context();
	if( ctx )
		ctx->renderbufferDeleted( this );
	
	if( mId )
		glDeleteRenderbuffers( 1, &mId );
}

void Renderbuffer::setLabel( const std::string &label )
{
	mLabel = label;
#if ! defined( CINDER_GL_ES_2 )
	env()->objectLabel( GL_RENDERBUFFER, mId, (GLsizei)label.size(), label.c_str() );
#endif
}

std::ostream& operator<<( std::ostream &os, const Renderbuffer &rhs )
{
	os << "ID: " << rhs.mId << std::endl;
	if( ! rhs.mLabel.empty() )
		os << "       Label: " << rhs.mLabel << std::endl;
	os << "  Intrnl Fmt: " << constantToString( rhs.getInternalFormat() );
	os << "    Dims: " << rhs.mWidth << " x " << rhs.mHeight << std::endl;	
	
	return os;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo::Format
Fbo::Format::Format()
{
	mColorBufferInternalFormat   = getDefaultColorInternalFormat();
	mDepthBufferInternalFormat   = getDefaultDepthInternalFormat();

	mColorTextureFormat   = getDefaultColorTextureFormat( true );
	mDepthTextureFormat   = getDefaultDepthTextureFormat();

	// Color
	mColorTexture = true;
	mColorBuffer = false;	
	// Depth
	mDepthBuffer = true;
	mDepthTexture = false;
	// Stencil
	mStencilTexture = false;	
	mStencilBuffer = false;

	mSamples = 0;
	mCoverageSamples = 0;
	
	mAutoResolve = true;
	mAutoMipmap = true;
}

GLenum Fbo::Format::getDefaultColorInternalFormat( bool alpha )
{
#if defined( CINDER_GL_ES_2 )
	return GL_RGBA;
#else
	return GL_RGBA8;
#endif
}

GLenum Fbo::Format::getDefaultDepthInternalFormat()
{
#if defined( CINDER_GL_ES_2 )
	return GL_DEPTH_COMPONENT24_OES;
#else
	return GL_DEPTH_COMPONENT24;
#endif
}

Texture::Format	Fbo::Format::getDefaultColorTextureFormat( bool alpha )
{
#if defined( CINDER_GL_ES_2 )
	auto internalFormat = alpha ? GL_RGBA8_OES : GL_RGB8_OES;
#else
	auto internalFormat = alpha ? GL_RGBA8 : GL_RGB8;
#endif

	return Texture::Format().internalFormat( internalFormat ).immutableStorage();
}

Texture::Format	Fbo::Format::getDefaultDepthTextureFormat()
{
#if defined( CINDER_GL_ES_2 )
	return Texture::Format().internalFormat( GL_DEPTH_COMPONENT24_OES ).immutableStorage();
#else
	return Texture::Format().internalFormat( GL_DEPTH_COMPONENT24 ).immutableStorage().swizzleMask( GL_RED, GL_RED, GL_RED, GL_ONE );
#endif
}

// Returns the +stencil complement of a given internalFormat; ie GL_DEPTH_COMPONENT24 -> GL_DEPTH_COMPONENT24
void Fbo::Format::getDepthStencilFormats( GLint depthInternalFormat, GLint *resultInternalFormat, GLenum *resultPixelDataType )
{
	switch( depthInternalFormat ) {
#if defined( CINDER_GL_ES_2 )
		case GL_DEPTH24_STENCIL8_OES:
			*resultInternalFormat = GL_DEPTH24_STENCIL8_OES; *resultPixelDataType = GL_UNSIGNED_INT_24_8_OES;
		break;
		case GL_DEPTH_STENCIL_OES:
			*resultInternalFormat = GL_DEPTH24_STENCIL8_OES; *resultPixelDataType = GL_UNSIGNED_INT_24_8_OES;
		break;
		case GL_DEPTH_COMPONENT:
			*resultInternalFormat = GL_DEPTH24_STENCIL8_OES; *resultPixelDataType = GL_UNSIGNED_INT_24_8_OES;
		break;
		case GL_DEPTH_COMPONENT24_OES:
			*resultInternalFormat = GL_DEPTH24_STENCIL8_OES; *resultPixelDataType = GL_UNSIGNED_INT_24_8_OES;
		break;
#else
		case GL_DEPTH24_STENCIL8:
			*resultInternalFormat = GL_DEPTH24_STENCIL8; *resultPixelDataType = GL_UNSIGNED_INT_24_8;
		break;
		case GL_DEPTH32F_STENCIL8:
			*resultInternalFormat = GL_DEPTH32F_STENCIL8; *resultPixelDataType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		break;
		case GL_DEPTH_COMPONENT24:
			*resultInternalFormat = GL_DEPTH24_STENCIL8; *resultPixelDataType = GL_UNSIGNED_INT_24_8;
		break;
		case GL_DEPTH_COMPONENT32F:
			*resultInternalFormat = GL_DEPTH32F_STENCIL8; *resultPixelDataType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		break;
#endif

		default:
			*resultInternalFormat = GL_INVALID_ENUM; *resultPixelDataType = GL_INVALID_ENUM;
		break;
	}
}

/*
Fbo::Format& Fbo::Format::attachment( GLenum attachmentPoint, const RenderbufferRef &buffer, RenderbufferRef multisampleBuffer )
{
	mAttachmentsBuffer[attachmentPoint] = buffer;
	mAttachmentsMultisampleBuffer[attachmentPoint] = multisampleBuffer;
	mAttachmentsTexture.erase( attachmentPoint );
	return *this;
}

Fbo::Format& Fbo::Format::attachment( GLenum attachmentPoint, const TextureBaseRef &texture, RenderbufferRef multisampleBuffer )
{
	mAttachmentsTexture[attachmentPoint] = texture;
	mAttachmentsMultisampleBuffer[attachmentPoint] = multisampleBuffer;
	mAttachmentsBuffer.erase( attachmentPoint );
	return *this;
}
*/

Fbo::Format& Fbo::Format::attachment( GLenum attachmentPoint, const TextureBaseRef &texture, const TextureBaseRef &resolve )
{
	Fbo::AttachmentRef attachment = Fbo::Attachment::create( texture, resolve );
	mAttachments[attachmentPoint] = attachment;
	return *this;
}

Fbo::Format& Fbo::Format::attachment( GLenum attachmentPoint, const RenderbufferRef &buffer, const TextureBaseRef &resolve )
{
	Fbo::AttachmentRef attachment = Fbo::Attachment::create( buffer, resolve );
	mAttachments[attachmentPoint] = attachment;
	return *this;
}

Fbo::Format& Fbo::Format::removeAttachment( GLenum attachmentPoint )
{
	mAttachments.erase( attachmentPoint );
	return *this;
	
/*
	mAttachmentsBuffer.erase( attachmentPoint );
	mAttachmentsMultisampleBuffer.erase( attachmentPoint );	
	mAttachmentsTexture.erase( attachmentPoint );
*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fbo
FboRef Fbo::create( int width, int height, const Format &format )
{
	return FboRef( new Fbo( width, height, format ) );
}

FboRef Fbo::create( int width, int height, bool alpha, bool depth, bool stencil )
{
	Fbo::Format format;
	format.mColorTextureFormat = Format::getDefaultColorTextureFormat( alpha );
	format.mDepthBuffer = depth;
	format.mStencilBuffer = stencil;

	return FboRef( new Fbo( width, height, format ) );
}

Fbo::Fbo( int width, int height, const Format &format )
	: mWidth( width ), mHeight( height ), mFormat( format ), mId( 0 ), mMultisampleFramebufferId( 0 ),
	mHasColorAttachments( false ),	mHasDepthAttachment( false ), mHasStencilAttachment( false ), mHasMultisampleTexture( false )
{
	init();
	gl::context()->framebufferCreated( this );

#if defined( CINDER_ANDROID )
	CI_LOG_I( "Fbo::Fbo size=" << width << "x" << height) ;
#endif	
}

Fbo::~Fbo()
{
	auto ctx = gl::context();
	if( ctx )
		ctx->framebufferDeleted( this );

	if( mId )
		glDeleteFramebuffers( 1, &mId );
	if( mMultisampleFramebufferId )
		glDeleteFramebuffers( 1, &mMultisampleFramebufferId );
}

struct Counts {
	uint8_t						mNumColorTexture1D;
	uint8_t						mNumColorTexture2D;
	uint8_t						mNumColorTexture3D;
	uint8_t						mNumColorBuffer;
	uint8_t						mNumDepthTexture;
	uint8_t						mNumDepthBuffer;
	uint8_t						mNumStencilTexture;
	uint8_t						mNumStencilBuffer;
	uint8_t						mNumDepthStencilTexture;
	uint8_t						mNumDepthStencilBuffer;
	uint8_t						mNumTextureArray;
	std::map<GLenum, int32_t>	mSampleCounts;
	uint32_t					mMinArraySize;
	uint32_t					mMaxArraySize;
	uint32_t					mNumFixedSampleLocations;
	GLenum						mDepthTextureInternalFormat;
	GLenum						mDepthBufferInternalFormat;
	GLenum						mStencilTextureInternalFormat;
	GLenum						mStencilBufferInternalFormat;
	GLenum						mDepthStencilTextureInternalFormat;
	GLenum						mDepthStencilBufferInternalFormat;
};

void countAttachments( const std::map<GLenum, Fbo::AttachmentRef>& attachments, Counts *outCounts ) 
{
	Counts counts = {};
	counts.mDepthTextureInternalFormat			= GL_INVALID_ENUM;
	counts.mDepthBufferInternalFormat			= GL_INVALID_ENUM;
	counts.mStencilTextureInternalFormat		= GL_INVALID_ENUM;
	counts.mStencilBufferInternalFormat			= GL_INVALID_ENUM;
	counts.mDepthStencilTextureInternalFormat	= GL_INVALID_ENUM;
	counts.mDepthStencilBufferInternalFormat	= GL_INVALID_ENUM;
	
	for( const auto &it : attachments ) {
		GLenum attachmentPoint = it.first;
		const auto &attachment = it.second;
		// Sample count
		counts.mSampleCounts[attachmentPoint] = static_cast<uint32_t>( attachment->getSamples() );
		// Process properties
		GLenum internalFormat = attachment->getInternalFormat();
		GLenum aspect = determineAspectFromFormat( internalFormat );
		switch( aspect ) {
			// Color
			case GL_COLOR: {
				const auto& attachment = it.second;
				GLenum target = attachment->getTexture() ? attachment->getTexture()->getTarget() : GL_INVALID_ENUM;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
				switch( target ) {
					case GL_TEXTURE_1D: ++counts.mNumColorTexture1D; break;
					case GL_TEXTURE_2D_MULTISAMPLE:
					case GL_TEXTURE_2D: ++counts.mNumColorTexture2D; break;
					case GL_TEXTURE_3D: ++counts.mNumColorTexture3D; break;
					case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
					case GL_TEXTURE_2D_ARRAY: {
						++(counts.mNumTextureArray);
						uint32_t depth = attachment->getTexture()->getDepth();
						counts.mMinArraySize = ( 0 == counts.mMinArraySize ) ? depth : std::min( counts.mMinArraySize, depth );
						counts.mMaxArraySize = ( 0 == counts.mMaxArraySize ) ? depth : std::max( counts.mMaxArraySize, depth );
					}
					case GL_RENDERBUFFER: ++counts.mNumColorBuffer; break;
				}
#else
				switch( target ) {
	#if ! defined( CINDER_GL_ANGLE )
					case GL_TEXTURE_1D: ++counts.mNumColorTexture1D; break;
	#endif
					case GL_TEXTURE_2D: ++counts.mNumColorTexture2D; break;
					case GL_TEXTURE_3D: ++counts.mNumColorTexture3D; break;
					case GL_TEXTURE_2D_ARRAY: {
						++counts.mNumTextureArray;
						uint32_t depth = attachment->getTexture()->getDepth();
						counts.mMinArraySize = ( 0 == counts.mMinArraySize ) ? depth : std::min( counts.mMinArraySize, depth );
						counts.mMaxArraySize = ( 0 == counts.mMaxArraySize ) ? depth : std::max( counts.mMaxArraySize, depth );
					}
					break;
					case GL_RENDERBUFFER: ++counts.mNumColorBuffer; break;
				}
#endif
			}
			break;
			// Depth
			case GL_DEPTH: {
				if( attachment->isTexture() ) {
					++counts.mNumDepthTexture;
					counts.mDepthTextureInternalFormat = internalFormat;
				}
				else if( attachment->isBuffer() ) {
					++counts.mNumDepthBuffer;
					counts.mDepthBufferInternalFormat = internalFormat;
				}
			}
			break;
			// Stencil
			case GL_STENCIL: {
				if( attachment->isTexture() ) {
					++counts.mNumStencilTexture;
					counts.mStencilTextureInternalFormat = internalFormat;
				}
				else if( attachment->isBuffer() ) {
					++counts.mNumStencilBuffer;
					counts.mStencilBufferInternalFormat = internalFormat;
				}
			}
			break;
			// Depth stencil
			case GL_DEPTH_STENCIL: {
				if( attachment->isTexture() ) {
					++counts.mNumDepthStencilTexture;
					counts.mDepthStencilTextureInternalFormat = internalFormat;
				}
				else if( attachment->isBuffer() ) {
					++counts.mNumDepthStencilBuffer;
					counts.mDepthStencilBufferInternalFormat = internalFormat;
				}
			}
			break;
		}

#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
		if( attachment->getTexture() && ( typeid(*(attachment->getTexture())) == typeid(Texture2d) ) ) {
			auto tex = std::dynamic_pointer_cast<Texture2d>( attachment->getTexture() );
			if( tex->getForamt().isFixedSampleLocations() ) {
				++counts.mNumFixedSampleLocations;
			}
		}
#endif 
	}

	*outCounts = counts;
}

void validate( const Fbo::Format &mFormat, const Counts& counts, bool *outHasColor, bool *outHasDepth, bool *outHasStencil, bool *outHasArray, int32_t *outSampleCount, bool *outHasMultisampleTexture )
{
	bool formatHas1D = false;
	bool formatHas2D = mFormat.hasColorBuffer();
	bool formatHas3D = false;
	bool formatHasArray = false;
	if( mFormat.hasColorTexture() ) {
		switch( mFormat.getColorTextureFormat().getTarget() ) {
#if ! defined( CINDER_GL_ANGLE )
			case GL_TEXTURE_1D: formatHas1D |= true; break;
#endif
			case GL_TEXTURE_2D: formatHas2D |= true; break;
			case GL_TEXTURE_3D: formatHas3D |= true; break;
			case GL_TEXTURE_2D_ARRAY: formatHasArray |= true; break;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
			case GL_TEXTURE_2D_MULTISAMPLE: formatHas2D |= true; break;
			case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: formatHasArray |= true; break;
#endif
		}
	}

	bool has1D = ( counts.mNumColorTexture1D > 0 );
	bool has2D = ( ( counts.mNumColorTexture2D > 0 ) || ( counts.mNumColorBuffer > 0 ) );
	bool has3D = ( counts.mNumColorTexture3D > 0 );
	bool hasArray = ( counts.mNumTextureArray > 0 );

	// Cannot mix target types
	{
		bool isInvalid = false;
		isInvalid |= ( formatHas1D && formatHas2D ) || ( formatHas1D && formatHas3D ) || ( formatHas1D && formatHasArray ) || ( formatHas2D && formatHas3D ) || ( formatHas2D && formatHasArray ) || ( formatHas3D && formatHasArray );
		isInvalid |= ( has1D && has2D ) || ( has1D && has3D ) || ( has1D && hasArray ) || ( has2D && has3D ) || ( has2D && hasArray ) || ( has3D && hasArray );
		if( isInvalid ) {
			throw FboException( "Cannot mix target types" );
		}
	}

#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
	// Samples must be same for all attachments
	if( counts.mSampleCounts.size() >= 2 ) {
		auto iter0 = counts.mSampleCounts.begin();
		auto iter1 = counts.mSampleCounts.begin();
		++iter1;
		bool notSame = false;
		for( ; iter1 != counts.mSampleCounts.end(); ++iter0, ++iter1 ) {
			if( iter0->second != iter1->second ) {
				std::stringstream ss;
				ss << "(" << gl::constantToString( iter0->first ) << " has " << iter0->second << ", but " << gl::constantToString( iter1->first ) << " has " << iter1->second << " samples" << ")";
				throw FboException( "Samples must be same for all attachments" + ss.str() );
			}
		}
	}

	// Fixed sample locations for all texture attachments when used with renderbuffers
	bool formatHasAnyTexture = mFormat.hasColorTexture() || mFormat.hasDepthTexture() || mFormat.hasStencilTexture();
	bool formatHasAnyBuffer = mFormat.hasColorBuffer() || mFormat.hasDepthBuffer() || mFormat.hasStencilBuffer();
	bool hasAnyTexture = ( counts.mNumColorTexture2D > 0 ) || ( counts.mNumDepthTexture > 0 ) || ( counts.mNumStencilTexture > 0 );
	bool hasAnyBuffer = ( counts.mNumColorBuffer > 0 ) || ( counts.mNumDepthBuffer > 0 ) || ( counts.mNumStencilBuffer > 0 ) || ( counts.mNumDepthStencilBuffer > 0 );
	{
		bool isMultiSample = false; 
		isMultiSample |= ( mFormat.getSamples() > 1 );
		isMultiSample |= ( mFormat.hasColorTexture() && mFormat.getColorTextureFormat().isMultisample() );
		isMultiSample |= ( mFormat.hasDepthTexture() && mFormat.getDepthTextureFormat().isMultisample() );
		isMultiSample |= ( ( ! counts.mSampleCounts.empty() ) && ( counts.mSampleCounts.begin()->second > 1 ) );

		uint32_t formatTexCount = 0;
		formatTexCount += mFormat.hasColorTexture()  ? 1 : 0;
		formatTexCount += mFormat.hasDepthTexture()  ? 1 : 0;

		uint32_t texCount = 0;
		texCount += counts.mNumColorTexture2D;
		texCount += counts.mNumDepthTexture;
		texCount += counts.mNumStencilTexture;

		uint32_t formatFixSampCount = 0;
		formatFixSampCount += ( mFormat.hasColorTexture() && mFormat.getColorTextureFormat().isFixedSampleLocations() ) ? 1 : 0;
		formatFixSampCount += ( mFormat.hasDepthTexture() && mFormat.getDepthTextureFormat().isFixedSampleLocations() ) ? 1 : 0;

		bool isInvalid = false;
		isInvalid |= isMultiSample && formatHasAnyTexture  && formatHasAnyBuffer && ( formatTexCount != formatFixSampCount );
		isInvalid |= isMultiSample && hasAnyTexture  && hasAnyBuffer && ( texCount != counts.mNumFixedSampleLocations );
		if( isInvalid ) {
			throw FboException( "Fixed sample locations required for all texture attachments when used with renderbuffers" );
		}
	}
#endif

	// Depth and stencil must use combined format if both are used
	bool formatHasDepthStencil = ( mFormat.hasDepthTexture() && mFormat.hasStencilTexture() ) || ( mFormat.hasDepthBuffer() && mFormat.hasStencilBuffer() ); //( ( mFormat.hasDepthTexture() || mFormat.hasDepthBuffer() ) && ( mFormat.mStencilTexture || mFormat.hasStencilBuffer() ) );
	bool formatHasDepth = mFormat.hasDepthTexture() || mFormat.hasDepthBuffer();
	bool formatHasStencil = mFormat.hasStencilTexture() || mFormat.hasStencilBuffer();
	bool hasDepthStencil = ( counts.mNumDepthStencilTexture > 0 ) || ( counts.mNumDepthStencilBuffer > 0 );
	bool hasDepth = ( counts.mNumDepthTexture > 0 ) || ( counts.mNumDepthBuffer > 0 );
	bool hasStencil = ( counts.mNumStencilTexture > 0 ) || ( counts.mNumStencilBuffer > 0 );
	{
		bool isInvalid = false;
		isInvalid |= ( hasDepth && hasStencil && ( ! hasDepthStencil ) );
		isInvalid |= ( formatHasDepth && formatHasStencil && ( ! formatHasDepthStencil ) );
		if( isInvalid ) {
			throw FboException( "Depth and stencil must use combined format if both are present" );
		}
	}

	// GL_TEXTURE_3D targets do not support any depth or stencil attachments
	bool hasAnyDepthStencilTexture = ( counts.mNumDepthTexture > 0 ) || ( counts.mNumStencilTexture > 0 ) || ( counts.mNumDepthStencilTexture > 0 ) || mFormat.hasDepthTexture() || mFormat.hasStencilTexture();
	bool hasAnyDepthStencilBuffer = ( counts.mNumDepthBuffer > 0 ) || ( counts.mNumStencilBuffer > 0 ) || ( counts.mNumDepthStencilBuffer > 0 ) || mFormat.hasDepthBuffer() || mFormat.hasStencilBuffer();
	if( has3D && ( hasAnyDepthStencilTexture || hasAnyDepthStencilBuffer ) ) {
		throw FboException( "GL_TEXTURE_3D targets do not support any depth or stencil attachments" );
	}

	// Check depth/stencil attachment internalFormats. 
	//
	// NOTE: This is aggressive because on certain platforms (NVIDIA) the 
	//       internalFormat can affect the framebuffer's completeness status.
	// 
	{
		// Depth
		{
			GLenum internalFormat = 0;
			if( hasDepth ) {
				internalFormat = ( GL_INVALID_ENUM != counts.mDepthTextureInternalFormat ) ? counts.mDepthTextureInternalFormat : internalFormat;
				internalFormat = ( GL_INVALID_ENUM != counts.mDepthBufferInternalFormat ) ? counts.mDepthBufferInternalFormat : internalFormat;
			}
			else if( formatHasDepth ) {
				internalFormat = ( mFormat.hasDepthTexture() ) ? mFormat.getDepthTextureFormat().getInternalFormat() : internalFormat;
				internalFormat = ( mFormat.hasDepthBuffer() ) ? mFormat.getDepthBufferInternalFormat() : internalFormat;
			}

			if( 0 != internalFormat ) {
				bool isInvalid = false;
				isInvalid |= ( GL_DEPTH != determineAspectFromFormat( internalFormat ) );
				isInvalid &= ( GL_DEPTH_STENCIL != determineAspectFromFormat( internalFormat ) );
				if( isInvalid ) {
					throw FboException( "Invalid internal format for depth " + gl::constantToString( internalFormat ) );
				}
			}
		}

		// Stencil
		{
			GLenum internalFormat = 0;
			if( hasStencil ) {
				internalFormat = ( GL_INVALID_ENUM != counts.mStencilTextureInternalFormat ) ? counts.mStencilTextureInternalFormat : internalFormat;
				internalFormat = ( GL_INVALID_ENUM != counts.mStencilBufferInternalFormat ) ? counts.mStencilBufferInternalFormat : internalFormat;
			}

			if( 0 != internalFormat ) {
				bool isInvalid = false;
				isInvalid |= ( GL_STENCIL != determineAspectFromFormat( internalFormat ) );
				isInvalid &= ( GL_DEPTH_STENCIL != determineAspectFromFormat( internalFormat ) );
				if( isInvalid ) {
					throw FboException( "Invalid internal format for stencil " + gl::constantToString( internalFormat ) );
				}
			}
		}

		// Depth/stencil
		{
			GLenum internalFormat = 0;
			if( hasDepthStencil ) {
				internalFormat = ( GL_INVALID_ENUM != counts.mDepthStencilTextureInternalFormat ) ? counts.mDepthStencilTextureInternalFormat : internalFormat;
				internalFormat = ( GL_INVALID_ENUM != counts.mDepthStencilBufferInternalFormat ) ? counts.mDepthStencilBufferInternalFormat : internalFormat;
			}
			else if( formatHasDepthStencil ) {
				// The ambiguity of all the depth/stencil texture and buffer combinations should be resolved at this point
				GLint resultInternalFormat = GL_INVALID_ENUM;
				GLenum resultPixelDataType = GL_INVALID_ENUM;
				if( mFormat.hasDepthTexture() && mFormat.hasStencilTexture() ) {
					Fbo::Format::getDepthStencilFormats( mFormat.getDepthTextureFormat().getInternalFormat(), &resultInternalFormat, &resultPixelDataType );
				}
				else if( mFormat.hasDepthBuffer() && mFormat.hasStencilBuffer() ) {
					Fbo::Format::getDepthStencilFormats( mFormat.getDepthBufferInternalFormat(), &resultInternalFormat, &resultPixelDataType );
				}
				internalFormat = ( GL_INVALID_ENUM != resultInternalFormat ) ? resultInternalFormat : internalFormat;
			}

			if( 0 != internalFormat) {
				bool isInvalid = false;
				isInvalid |= ( GL_DEPTH_STENCIL != determineAspectFromFormat( internalFormat ) );
				if( isInvalid ) {
					throw FboException( "Invalid internal format for DepthStencil " + gl::constantToString( internalFormat ) );
				}
			}
		}
	}

	// Write output
	*outHasColor    = has1D || has2D || has3D || hasArray;
	*outHasDepth    = ( counts.mNumDepthTexture > 0 ) || ( counts.mNumDepthBuffer > 0 ) || ( counts.mNumDepthStencilTexture > 0 ) || ( counts.mNumDepthStencilBuffer > 0 );
	*outHasStencil  = ( counts.mNumStencilTexture > 0 ) || ( counts.mNumStencilBuffer > 0 ) || ( counts.mNumDepthStencilTexture > 0 ) || ( counts.mNumDepthStencilBuffer > 0 );
	*outHasArray    = hasArray;
	*outSampleCount = ( ! counts.mSampleCounts.empty() ) ? counts.mSampleCounts.begin()->second : -1;
	*outHasMultisampleTexture = ( has2D || hasArray ) && ( *outSampleCount > 1 );
}

void Fbo::init()
{
    // Use renderbuffers when multiple sample is requested on platforms that do not support multisample textures
    if( mFormat.mColorTexture && mFormat.mAttachments.empty() && ( mFormat.mSamples > 1 ) && ( ! gl::env()->supportsTextureMultisample() ) ) {
        mFormat.mColorTexture = false;
        mFormat.mColorBuffer = true;
    }

#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
	// NOTE: Force requested depth and stencil to texture if multisample 
	//       textures are requested so we don't end up with a bunch of 
	//       needless mixed format exceptions.
	//
	if( mFormat.mColorTexture && ( ( mFormat.mSamples > 1 ) || ( mFormat.mColorTextureFormat.isMultisample() ) ) ) {
		// Depth
		if( mFormat.mDepthBuffer ) {
			mFormat.mDepthBuffer = false;
			mFormat.mDepthTexture = true;
		}
		// Stencil
		if( mFormat.mStencilBuffer ) {
			mFormat.mStencilBuffer = false;
			mFormat.mStencilTexture = true;
		}
	}
#endif

	// NOTE: Prefer to use a texture if both depth and stencil are requested to 
	//       avoid any ambiguity between the various depth/stencil texture or 
	//       buffer combinations. 
	//
	if( mFormat.mStencilBuffer && mFormat.mDepthTexture ) {
		mFormat.mStencilBuffer = false;
		mFormat.mDepthBuffer = false;
		mFormat.mStencilTexture = true;
	}

	// Adjust the depth internal format if both depth and stencil are requested
	if( ( mFormat.mDepthTexture || mFormat.mDepthBuffer ) && ( mFormat.mStencilTexture || mFormat.mStencilBuffer ) ) {
		// Texture
		GLint resultInternalFormat = GL_INVALID_ENUM;
		GLenum resultPixelDataType = GL_INVALID_ENUM;
		Fbo::Format::getDepthStencilFormats( mFormat.mDepthTextureFormat.getInternalFormat(), &resultInternalFormat, &resultPixelDataType );
		mFormat.mDepthTextureFormat.setInternalFormat( resultInternalFormat );
		// Buffer
		resultInternalFormat = GL_INVALID_ENUM;
		resultPixelDataType = GL_INVALID_ENUM;
		Fbo::Format::getDepthStencilFormats( mFormat.mDepthBufferInternalFormat, &resultInternalFormat, &resultPixelDataType );
		mFormat.mDepthBufferInternalFormat = resultInternalFormat;
	}

	mAttachments = mFormat.mAttachments;
	int32_t validationSampleCount = -1;
	Counts counts = {};
	countAttachments( mAttachments, &counts );
	validate( mFormat, counts, &mHasColorAttachments, &mHasDepthAttachment, &mHasStencilAttachment, &mHasArrayAttachment, &validationSampleCount, &mHasMultisampleTexture );

	if( -1 != validationSampleCount ) {
		mFormat.setSamples( validationSampleCount );
	}

#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE)
	if( mFormat.mOverrideTextureSamples ) {
		mFormat.mColorTextureFormat.setSamples( mFormat.getSamples() );
		mFormat.mDepthTextureFormat.setSamples( mFormat.getSamples() );
	}
#endif
	
	// NOTE: Force all requested buffers to textures if there is a multisample
	//       texture or an array attachment (single sample or multisample) 
	//       present.
	//
	if( mHasMultisampleTexture || mHasArrayAttachment ) {
		// Color
		if( mFormat.mColorBuffer ) {
			mFormat.mColorBuffer = false;
			mFormat.mColorTexture = true;
		}
		// Depth
		if( mFormat.mDepthBuffer ) {
			mFormat.mDepthBuffer = false;
			mFormat.mDepthTexture = true;
		}
		// Stencil
		if( mFormat.mStencilBuffer ) {
			mFormat.mStencilBuffer = false;
			mFormat.mStencilTexture = true;
		}
	}

	// allocate the framebuffer itself
	glGenFramebuffers( 1, &mId );
	ScopedFramebuffer fbScp( GL_FRAMEBUFFER, mId );

	// determine multisampling settings
	bool useMsaa = false;
	bool useCsaa = false;
	initMultisamplingSettings( &useMsaa, &useCsaa, &mFormat );

	// allocate the multisample framebuffer
	if( useMsaa || useCsaa ) {
		glGenFramebuffers( 1, &mMultisampleFramebufferId );
	}
	
	prepareAttachments( useMsaa || useCsaa );
	initDrawBuffers();
	updateActiveAttachments();
	attachAttachments();

	// mDrawBuffers is built in initDrawBuffers. 
	setDrawBuffers( mDrawBuffers );

/*
	if( useCsaa || useMsaa ) {
		initMultisample( mFormat );
	}

	
	setDrawBuffers( mId, mAttachmentsBuffer, mAttachmentsTexture );
	if( mMultisampleFramebufferId ) { // using multisampling and setup succeeded
		setDrawBuffers( mMultisampleFramebufferId, mAttachmentsMultisampleBuffer, map<GLenum,TextureBaseRef>() );
	}
*/		

	FboExceptionInvalidSpecification exc;
	if( ! checkStatus( &exc ) ) { // failed creation; throw
		throw exc;
	}
	
/*
	mNeedsResolve = false;
	mNeedsMipmapUpdate = false;
*/
	
	mLabel = mFormat.mLabel;
	if( ! mLabel.empty() ) {
		env()->objectLabel( GL_FRAMEBUFFER, mId, (GLsizei)mLabel.size(), mLabel.c_str() );
	}
}

void Fbo::initMultisamplingSettings( bool *useMsaa, bool *useCsaa, Format *format )
{
#if defined( CINDER_MSW ) && ( ! defined( CINDER_GL_ES ) )
	static bool csaaSupported = ( glext_NV_framebuffer_multisample_coverage != 0 );
#else
	static bool csaaSupported = false;
#endif
	*useCsaa = csaaSupported && ( format->mCoverageSamples > format->mSamples );
	*useMsaa = ( format->mCoverageSamples > 0 ) || ( format->mSamples > 0 );
	if( *useCsaa ) {
		*useMsaa = false;
	}

#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
	if( format->mColorTexture ) {
		*useCsaa = false;
		*useMsaa = ( format->mSamples > 1 ) || ( format->mColorTextureFormat.getSamples() > 1 );
	}
#endif

	// Cap samples to max samples
	format->mSamples = std::min( static_cast<GLint>( format->mSamples ), Fbo::getMaxSamples() );
	if( format->mOverrideTextureSamples ) {
		// Color
		format->mColorTextureFormat.setSamples( format->mSamples );
		// Depth
		format->mColorTextureFormat.setSamples( format->mSamples );
	}
	
	// For multisample textures - disable immutable storage if not supported
	bool supportsTextureStorageMultisample = gl::env()->supportsTextureStorageMultisample();
	if( ! supportsTextureStorageMultisample ) {
		// Color
		if( format->mColorTextureFormat.getSamples() > 1 ) {
			format->mColorTextureFormat.setImmutableStorage( false );
		}
		// Depth
		if( format->mDepthTextureFormat.getSamples() > 1 ) {
			format->mDepthTextureFormat.setImmutableStorage( false );
		}
	}
}

// Iterate the Format's requested attachments and create any we don't already have attachments for
#if defined( CINDER_GL_ES_2 )
void Fbo::prepareAttachments( const Fbo::Format &format, bool multisampling )
{
/*
	mAttachmentsBuffer = format.mAttachmentsBuffer;
	mAttachmentsTexture = format.mAttachmentsTexture;

	// Create the default color attachment if there's not already something on GL_COLOR_ATTACHMENT0
	bool preexistingColorAttachment = mAttachmentsTexture.count( GL_COLOR_ATTACHMENT0 ) || mAttachmentsBuffer.count( GL_COLOR_ATTACHMENT0 );
	if( format.mColorTexture && ( ! preexistingColorAttachment ) ) {
		mAttachmentsTexture[GL_COLOR_ATTACHMENT0] = Texture::create( mWidth, mHeight, format.mColorTextureFormat );
	}
	
	// Create the default depth(+stencil) attachment if there's not already something on GL_DEPTH_ATTACHMENT || GL_DEPTH_STENCIL_ATTACHMENT
#if defined( CINDER_GL_ES_2 )
	bool preexistingDepthAttachment = mAttachmentsTexture.count( GL_DEPTH_ATTACHMENT ) || mAttachmentsBuffer.count( GL_DEPTH_ATTACHMENT );
#else
	bool preexistingDepthAttachment = mAttachmentsTexture.count( GL_DEPTH_ATTACHMENT ) || mAttachmentsBuffer.count( GL_DEPTH_ATTACHMENT )
										|| mAttachmentsTexture.count( GL_DEPTH_STENCIL_ATTACHMENT ) || mAttachmentsBuffer.count( GL_DEPTH_STENCIL_ATTACHMENT );
#endif
	if( format.mDepthTexture && ( ! preexistingDepthAttachment ) ) {
#if ! defined( CINDER_LINUX_EGL_RPI2 )
		mAttachmentsTexture[GL_DEPTH_ATTACHMENT] = Texture::create( mWidth, mHeight, format.mDepthTextureFormat );
#else
		CI_LOG_W( "No depth texture support on the RPi2." );
#endif
	}
	else if( format.mDepthBuffer && ( ! preexistingDepthAttachment ) ) {
		if( format.mStencilBuffer ) {
			GLint internalFormat;
			GLenum pixelDataType;
			Format::getDepthStencilFormats( format.mDepthBufferInternalFormat, &internalFormat, &pixelDataType );
			RenderbufferRef depthStencilBuffer = Renderbuffer::create( mWidth, mHeight, internalFormat );
#if defined( CINDER_GL_ES_2 )
			mAttachmentsBuffer[GL_DEPTH_ATTACHMENT] = depthStencilBuffer;
			mAttachmentsBuffer[GL_STENCIL_ATTACHMENT] = depthStencilBuffer;
#else
			mAttachmentsBuffer[GL_DEPTH_STENCIL_ATTACHMENT] = Renderbuffer::create( mWidth, mHeight, internalFormat );
#endif
		}
		else {
			mAttachmentsBuffer[GL_DEPTH_ATTACHMENT] = Renderbuffer::create( mWidth, mHeight, format.mDepthBufferInternalFormat );
		}
	}
	else if( format.mStencilBuffer ) { // stencil only
		GLint internalFormat = GL_STENCIL_INDEX8;
		RenderbufferRef stencilBuffer = Renderbuffer::create( mWidth, mHeight, internalFormat );
		mAttachmentsBuffer[GL_STENCIL_ATTACHMENT] = stencilBuffer;
	}
*/
}
#else
void Fbo::prepareAttachments( bool multisample )
{
	// Create color attachment
	if( ! mHasColorAttachments ) {
		// Create attachment using texture
		if( mFormat.mColorTexture ) {
			TextureBaseRef texture;
			TextureBaseRef resolve;
			auto colorFormat = mFormat.mColorTextureFormat;
			if( multisample && ( mFormat.mSamples > 1 ) && ( 1 == colorFormat.getSamples() ) ) {
				colorFormat.setSamples( mFormat.mSamples );
			}
			texture = Texture::create( mWidth, mHeight, colorFormat );
			if( texture->getSamples() > 1 ) {
				auto resolveFormat = mFormat.mColorTextureFormat;
				resolveFormat.setSamples( 1 );
				resolve = Texture::create( mWidth, mHeight, resolveFormat );
			}
			addAttachment( GL_COLOR_ATTACHMENT0, texture, nullptr, resolve );
			mHasColorAttachments = true;
		}
		// Create attachment using renderbuffer
		else if( mFormat.mColorBuffer ) {
			// Create renderbuffer mirror FBO
			glGenFramebuffers( 1, &mMultisampleFramebufferId );
			ScopedFramebuffer fbScp( GL_FRAMEBUFFER, mMultisampleFramebufferId );
			// Create renderbuffer storage
			RenderbufferRef buffer = Renderbuffer::create( mWidth, mHeight, mFormat.mColorBufferInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
			auto resolveFormat = Texture2d::Format().internalFormat( mFormat.mColorBufferInternalFormat ).samples( 1 );
			TextureBaseRef resolve = Texture::create( mWidth, mHeight, resolveFormat );
			addAttachment( GL_COLOR_ATTACHMENT0, nullptr, buffer, resolve );
			mHasColorAttachments = true;
		}
	}

	// Create depth/stencil attachment
	bool needsDepth = ( ! mHasDepthAttachment ) && ( mFormat.mDepthTexture || mFormat.mDepthBuffer );
	bool needsStencil = ( ! mHasStencilAttachment ) && ( mFormat.mStencilTexture || mFormat.mStencilBuffer );
	// Depth/stencil
	if( needsDepth && needsStencil ) {
		if( mFormat.mDepthTexture || mFormat.mStencilTexture ) {
			auto depthStencilFormat = mFormat.mDepthTextureFormat;
			if( GL_DEPTH_STENCIL != determineAspectFromFormat( depthStencilFormat.getInternalFormat() ) ) {
				depthStencilFormat.setInternalFormat( GL_DEPTH24_STENCIL8 );
			}
			depthStencilFormat.setSamples( mFormat.mSamples );
			TextureBaseRef texture = Texture2d::create( mWidth, mHeight, depthStencilFormat );
			TextureBaseRef resolve;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
			if( depthStencilFormat.isMultisample() ) {
				auto resolveFormat = depthStencilFormat;
				resolveFormat.setSamples( 1 );
				resolve = Texture2d::create( mWidth, mHeight, resolveFormat );
			}
#endif
			addAttachment( GL_DEPTH_ATTACHMENT, texture, nullptr, resolve );
			mHasDepthAttachment = true;
		}
		else if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
			GLint internalFormat = GL_DEPTH24_STENCIL8;
			RenderbufferRef buffer = Renderbuffer::create( mWidth, mHeight, internalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
			TextureBaseRef resolve = Texture2d::create( mWidth, mHeight, Texture2d::Format().internalFormat( internalFormat ).samples( 1 ) );
			addAttachment( GL_STENCIL_ATTACHMENT, nullptr, buffer, resolve );
		}
	}
	// Depth
	else if( needsDepth ) {
		if( mFormat.mDepthTexture ) {
			auto depthFormat = mFormat.mDepthTextureFormat;
			depthFormat.setSamples( mFormat.mSamples );
			TextureBaseRef texture = Texture2d::create( mWidth, mHeight, depthFormat );
			TextureBaseRef resolve;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
			if( depthFormat.isMultisample() ) {
				auto resolveFormat = depthFormat;
				resolveFormat.setSamples( 1 );
				resolve = Texture2d::create( mWidth, mHeight, resolveFormat );
			}
#endif
			addAttachment( GL_DEPTH_ATTACHMENT, texture, nullptr, resolve );
			mHasDepthAttachment = true;
		}
		else if( mFormat.mDepthBuffer ) {
			RenderbufferRef buffer = Renderbuffer::create( mWidth, mHeight, mFormat.mDepthBufferInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
			TextureBaseRef resolve = Texture2d::create( mWidth, mHeight, Texture2d::Format().internalFormat( mFormat.mDepthBufferInternalFormat ).samples( 1 ) );
			addAttachment( GL_DEPTH_ATTACHMENT, nullptr, buffer, resolve );
			mHasDepthAttachment = true;
		}
	}
	// Stencil
	else if( needsStencil ) {
		if( mFormat.mStencilTexture ) {
			auto stencilFormat = Texture2d::Format().internalFormat( GL_STENCIL_INDEX8 );
			stencilFormat.setSamples( mFormat.mSamples );
			TextureBaseRef texture = Texture2d::create( mWidth, mHeight, stencilFormat );
			TextureBaseRef resolve;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
			if( stencilFormat.isMultisample() ) {
				auto resolveFormat = stencilFormat;
				resolveFormat.setSamples( 1 );
				resolve = Texture2d::create( mWidth, mHeight, resolveFormat );
			}
#endif
			addAttachment( GL_STENCIL_ATTACHMENT, texture, nullptr, resolve );
			mHasStencilAttachment = true;
		}
		else if( mFormat.mStencilBuffer ) {
			GLint internalFormat = GL_STENCIL_INDEX8;
			RenderbufferRef buffer = Renderbuffer::create( mWidth, mHeight, internalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
			TextureBaseRef resolve = Texture2d::create( mWidth, mHeight, Texture2d::Format().internalFormat( internalFormat ).samples( 1 ) );
			addAttachment( GL_STENCIL_ATTACHMENT, nullptr, buffer, resolve );
		}
	}
}
#endif

#if defined( CINDER_GL_ES )
void Fbo::attachAttachments()
{
	// Attach images
	{
		ScopedFramebuffer fbScp( GL_FRAMEBUFFER, ( 0 != mMultisampleFramebufferId ) ? mMultisampleFramebufferId : mId );

		for( auto &it : mAttachments ) {
			GLenum attachmentPoint = it.first;
			auto& attachment = it.second;
			if( attachment->mTexture ) {				
				GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
				GLuint id = attachment->mTexture->getId();
				if( glFramebufferTexture ) {
					if( GL_TEXTURE_CUBE_MAP == target ) {
						glFramebufferTexture2D( GL_FRAMEBUFFER, attachmentPoint, target, id, 0 );
					}
					else {
						glFramebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
					}
				}
				else {
					GLenum target = attachment->mTexture->getTarget();
					glFramebufferTexture2D( GL_FRAMEBUFFER, attachmentPoint, target, id, 0 );
				}
			}
			else if( attachment->mBuffer ) {
				GLuint id = attachment->mBuffer->getId();
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, id );
			}
		}
	}

	// Attach resolves
	{
		ScopedFramebuffer fbScp( GL_FRAMEBUFFER, mId );

		for( auto &it : mAttachments ) {
			GLenum attachmentPoint = it.first;
			auto& attachment = it.second;
			if( ! attachment->mResolve ) {
				continue;
			}

			GLenum target = attachment->mResolve->getTarget();
			GLuint id = attachment->mResolve->getId();
			if( glFramebufferTexture ) {
				glFramebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
			}
			else {
				glFramebufferTexture2D( GL_FRAMEBUFFER, attachmentPoint, target, id, 0 );
			}
		}
	}
}
#else
void Fbo::attachAttachments()
{
	// Attach images
	{
		ScopedFramebuffer fbScp( GL_FRAMEBUFFER, ( 0 != mMultisampleFramebufferId ) ? mMultisampleFramebufferId : mId );

		for( auto &it : mAttachments ) {
			GLenum attachmentPoint = it.first;
			auto& attachment = it.second;
			if( attachment->mTexture ) {
				GLenum target = attachment->mTexture->getTarget();
				GLuint id = attachment->mTexture->getId();
				if( GL_TEXTURE_CUBE_MAP == target ) {
					target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
					//framebufferTexture2d( GL_FRAMEBUFFER, attachmentPoint, target, id, 0 );
					glFramebufferTexture2D( GL_FRAMEBUFFER, attachmentPoint, target, id, 0 );
				}
				else {
					//framebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
					glFramebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
				}
			}
			else if( attachment->mBuffer ) {
				GLuint id = attachment->mBuffer->getId();
				//framebufferRenderbuffer( GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, id );
				glFramebufferRenderbuffer( GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, id );
			}
		}
	}

	// Attach resolves
	{
		ScopedFramebuffer fbScp( GL_FRAMEBUFFER, mId );

		for( auto &it : mAttachments ) {
			GLenum attachmentPoint = it.first;
			auto& attachment = it.second;
			if( ! attachment->mResolve ) {
				continue;
			}
			GLuint id = attachment->mResolve->getId();
			//framebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
			glFramebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
		}
	}
}
#endif

/*
// call glDrawBuffers against all color attachments
void Fbo::setDrawBuffers( GLuint fbId, const map<GLenum,RenderbufferRef> &attachmentsBuffer, const map<GLenum,TextureBaseRef> &attachmentsTexture )
{
#if defined( CINDER_ANDROID ) 
	//CI_LOG_I( "mAttachmentsBuffer.size=" << mAttachmentsBuffer.size() );
	//CI_LOG_I( "mAttachmentsTexture.size=" << mAttachmentsTexture.size() );
#endif	

#if ! defined( CINDER_GL_ES_2 )
	ScopedFramebuffer fbScp( GL_FRAMEBUFFER, fbId );

	vector<GLenum> drawBuffers;
	for( const auto &bufferAttachment : attachmentsBuffer )
		if( bufferAttachment.first >= GL_COLOR_ATTACHMENT0 && bufferAttachment.first <= MAX_COLOR_ATTACHMENT )
			drawBuffers.push_back( bufferAttachment.first );

	for( const auto &textureAttachment : attachmentsTexture )
		if( textureAttachment.first >= GL_COLOR_ATTACHMENT0 && textureAttachment.first <= MAX_COLOR_ATTACHMENT )
			drawBuffers.push_back( textureAttachment.first );

	if( ! drawBuffers.empty() ) {
		std::sort( drawBuffers.begin(), drawBuffers.end() );
		glDrawBuffers( (GLsizei)drawBuffers.size(), &drawBuffers[0] );
	}
	else {
		GLenum none = GL_NONE;
		glDrawBuffers( 1, &none );
	}
#endif
}
*/

void Fbo::addAttachment( GLenum attachmentPoint, const TextureBaseRef &texture, const RenderbufferRef &buffer, const TextureBaseRef &resolve )
{
	// @TODO: Add validation check

	Fbo::AttachmentRef attachment;
	if( texture ) {
		attachment = Fbo::Attachment::create( texture, resolve );
	}
	else if( buffer ) {
		attachment = Fbo::Attachment::create( buffer, resolve );
	}

	if( attachment ) {
		mAttachments[attachmentPoint] = attachment;
	}
}

void Fbo::initDrawBuffers()
{
	mDrawBuffers.clear();
	for( auto &it : mAttachments ) {
		GLenum attachmentPoint = it.first;
		if( ( attachmentPoint >= GL_COLOR_ATTACHMENT0 ) && ( attachmentPoint < ( GL_COLOR_ATTACHMENT0 + Fbo::getMaxAttachments() ) ) ) {
			mDrawBuffers.push_back( attachmentPoint );
		}
	}
}

void Fbo::updateActiveAttachments()
{
	mActiveAttachments.clear();
	std::copy( std::begin( mDrawBuffers ), std::end( mDrawBuffers ), std::back_inserter( mActiveAttachments ) );

	if( mAttachments.end() != mAttachments.find( GL_DEPTH_ATTACHMENT ) ) {
		mActiveAttachments.push_back( GL_DEPTH_ATTACHMENT );
	}

	if( mAttachments.end() != mAttachments.find( GL_STENCIL_ATTACHMENT ) ) {
		mActiveAttachments.push_back( GL_STENCIL_ATTACHMENT );
	}

	if( mAttachments.end() != mAttachments.find( GL_DEPTH_STENCIL_ATTACHMENT ) ) {
		mActiveAttachments.push_back( GL_DEPTH_STENCIL_ATTACHMENT );
	}
}

void Fbo::attachment( GLenum attachmentPoint, const TextureBaseRef &texture, const TextureBaseRef &resolve )
{
	addAttachment( attachmentPoint, texture, nullptr, resolve );
}

void Fbo::attachment( GLenum attachmentPoint, const RenderbufferRef &buffer, const TextureBaseRef &resolve )
{
	addAttachment( attachmentPoint, nullptr, buffer, resolve );
}

void Fbo::detach( GLenum attachmentPoint )
{
	mAttachments.erase( attachmentPoint );
}

Texture2dRef Fbo::getColorTexture( GLenum attachmentPoint )
{
	Texture2dRef result;
	if( (  attachmentPoint >= GL_COLOR_ATTACHMENT0 ) && ( attachmentPoint < ( GL_COLOR_ATTACHMENT0 + Fbo::getMaxAttachments() ) ) ) {
		result = dynamic_pointer_cast<Texture2d>( getTextureBase( attachmentPoint ) );
	}
	return result;
}

#if defined( CINDER_GL_ES_2 )
Texture2dRef Fbo::getDepthTexture( GLenum attachmentPoint )
{
	Texture2dRef result;
	return result;
}
#else
Texture2dRef Fbo::getDepthTexture( GLenum attachmentPoint )
{
	Texture2dRef result;
	if( ( GL_DEPTH_ATTACHMENT == attachmentPoint ) || ( GL_DEPTH_STENCIL_ATTACHMENT == attachmentPoint ) ) {
		result = dynamic_pointer_cast<Texture2d>( getTextureBase( attachmentPoint ) );
	}
	return result;
}
#endif

#if ! defined( CINDER_GL_ES )
Texture2dRef Fbo::getStencilTexture( GLenum attachmentPoint )
{
	Texture2dRef result;
	if( ( GL_STENCIL_ATTACHMENT == attachmentPoint ) || ( GL_DEPTH_STENCIL_ATTACHMENT == attachmentPoint ) ) {
		result = dynamic_pointer_cast<Texture2d>( getTextureBase( attachmentPoint ) );
	}
	return result;
}
#endif

Texture2dRef Fbo::getDepthStencilTexture( GLenum attachmentPoint )
{
	Texture2dRef result;
	if( ( GL_DEPTH_STENCIL_ATTACHMENT == attachmentPoint ) || ( GL_DEPTH_ATTACHMENT == attachmentPoint ) || ( GL_STENCIL_ATTACHMENT == attachmentPoint ) ) {
		result = dynamic_pointer_cast<Texture2d>( getTextureBase( attachmentPoint ) );
	}
	return result;
}

/*
Texture2dRef Fbo::getDepthTexture()
{
	TextureBaseRef result;
	
	// search for a depth attachment
	auto attachedTextureIt = mAttachmentsTexture.find( GL_DEPTH_ATTACHMENT );
	if( attachedTextureIt != mAttachmentsTexture.end() )
		result = attachedTextureIt->second;
#if ! defined( CINDER_GL_ES_2 )
	else { // search for a depth+stencil attachment
		attachedTextureIt = mAttachmentsTexture.find( GL_DEPTH_STENCIL_ATTACHMENT );
		if( attachedTextureIt != mAttachmentsTexture.end() )
			result = attachedTextureIt->second;
	}
#endif
	if( result && ( typeid(*result) == typeid(Texture2d) ) ) {
		resolveTextures();
		updateMipmaps( attachedTextureIt->first );
        return static_pointer_cast<Texture2d>( result );
	}
	else
		return Texture2dRef();
}
*/

Texture2dRef Fbo::getTexture2d( GLenum attachmentPoint )
{
	return dynamic_pointer_cast<Texture2d>( getTextureBase( attachmentPoint ) );
}

TextureBaseRef Fbo::getTextureBase( GLenum attachmentPoint )
{
	TextureBaseRef result;
	auto it = mAttachments.find( attachmentPoint );
	if( it != mAttachments.end() ) {
		resolveTextures( attachmentPoint );
		updateMipmaps( attachmentPoint );
		auto& attachment = it->second;
		result = attachment->mResolve ? attachment->mResolve : attachment->mTexture;
	}
	return result;

}

void Fbo::setDrawBuffers( GLenum attachmentPoint )
{
	mDrawBuffers.clear();
	mDrawBuffers.push_back( attachmentPoint );

	glDrawBuffers( static_cast<GLsizei>( mDrawBuffers.size() ), mDrawBuffers.data() );

	updateActiveAttachments();
}

void Fbo::setDrawBuffers( const std::vector<GLenum> &attachmentPoints, bool sortAttachmentPoints )
{
#if ! defined( CINDER_GL_ES_2 )
	ScopedFramebuffer fbScp( GL_FRAMEBUFFER, mMultisampleFramebufferId ? mMultisampleFramebufferId : mId );
	
	mDrawBuffers = attachmentPoints;
	if( ! mDrawBuffers.empty() ) {
		if( sortAttachmentPoints ) {
			std::sort( std::begin( mDrawBuffers ), std::end( mDrawBuffers ) );
		}
		glDrawBuffers( static_cast<GLsizei>( mDrawBuffers.size() ), mDrawBuffers.data() );
	}
	else {
		mDrawBuffers.clear();
		GLenum none = GL_NONE;
		glDrawBuffers( 1, &none );
	}

	updateActiveAttachments();
#endif
}

void Fbo::bindTexture( int textureUnit, GLenum attachment )
{
	auto tex = getTextureBase( attachment );
	if( tex )
		tex->bind( textureUnit );
}

void Fbo::unbindTexture( int textureUnit, GLenum attachment )
{
	auto tex = getTextureBase( attachment );
	if( tex ) {
		tex->unbind( textureUnit );
	}
}

void Fbo::resolveTextures( GLenum attachmentPoint ) const
{
#if defined( CINDER_GL_ANGLE ) && ( ! defined( CINDER_GL_ES_3 ) )
	if( mMultisampleFramebufferId ) {
		ScopedFramebuffer drawFbScp( GL_DRAW_FRAMEBUFFER, mId );
		ScopedFramebuffer readFbScp( GL_READ_FRAMEBUFFER, mMultisampleFramebufferId );
		glBlitFramebufferANGLE( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST );
	}
#elif defined( CINDER_GL_HAS_FBO_MULTISAMPLING ) && defined( CINDER_GL_ES_2 )
	// iOS-specific multisample resolution code
	if( mMultisampleFramebufferId ) {
		ScopedFramebuffer drawFbScp( GL_DRAW_FRAMEBUFFER_APPLE, mId );
		ScopedFramebuffer readFbScp( GL_READ_FRAMEBUFFER_APPLE, mMultisampleFramebufferId );
		glResolveMultisampleFramebuffer();
	}
#elif defined( CINDER_GL_HAS_FBO_MULTISAMPLING )
	if( 0 != mMultisampleFramebufferId ) {
		// Build a map of attachments to resolve
		std::map<GLenum, Fbo::AttachmentRef> attachmentsToResolve;
		if( Fbo::ALL_ATTACHMENTS == attachmentPoint ) {
			for( auto &attachmentPoint : mActiveAttachments ) {
				auto it = mAttachments.find( attachmentPoint );
				if( it->second->mNeedsResolve ) {
					attachmentsToResolve[attachmentPoint] = it->second;
				}
			}
		}
		else {
			auto it = mAttachments.find( attachmentPoint );
			if( ( it != mAttachments.end() ) && it->second->mNeedsResolve ) {
				attachmentsToResolve[it->first] = it->second;
			}
		}

		if( ! attachmentsToResolve.empty() ) {
			auto ctx = context();
			ctx->pushFramebuffer( GL_DRAW_FRAMEBUFFER, mId );
			ctx->pushFramebuffer( GL_READ_FRAMEBUFFER, mMultisampleFramebufferId );

			// Process attachments
			for( auto& it : attachmentsToResolve ) {
				GLenum curAttachmentPoint = it.first;
				auto &curAttachment = it.second;

				glDrawBuffers( 1, &curAttachmentPoint );
				glReadBuffer( curAttachmentPoint );
#if ! defined( CINDER_GL_ANGLE )
				// ANGLE appears to error when requested to resolve a depth buffer
				glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
#else
				glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST );
#endif
				// Mark as resolved
				curAttachment->mNeedsResolve = false;
			}

			// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
			ctx->bindFramebuffer( GL_FRAMEBUFFER, mMultisampleFramebufferId );
			glDrawBuffers( static_cast<GLsizei>( mDrawBuffers.size() ), mDrawBuffers.data() );

			ctx->popFramebuffer( GL_DRAW_FRAMEBUFFER );
			ctx->popFramebuffer( GL_READ_FRAMEBUFFER );	
		}
	}
#endif
}

void Fbo::updateMipmaps( GLenum attachmentPoint ) const
{
	std::map<GLenum, Fbo::AttachmentRef> attachmentsToUpdate;
	if( Fbo::ALL_ATTACHMENTS == attachmentPoint ) {
		for( auto &attachmentPoint : mActiveAttachments ) {
			auto it = mAttachments.find( attachmentPoint );
			if( it->second->mNeedsMipmapUpdate ) {
				attachmentsToUpdate[attachmentPoint] = it->second;
			}
		}
	}
	else {
		auto it = mAttachments.find( attachmentPoint );
		if( ( it != mAttachments.end() ) && it->second->mNeedsMipmapUpdate ) {
			attachmentsToUpdate[it->first] = it->second;
		}
	}

	// Process attachments
	for( auto& it : attachmentsToUpdate ) {
		GLenum curAttachmentPoint = it.first;
		auto &curAttachment = it.second;

		auto &texture = curAttachment->mResolve ? curAttachment->mResolve : curAttachment->mTexture;
		ScopedTextureBind textureBind( texture );
		glGenerateMipmap( texture->getTarget() );

		// Mark as updated
		curAttachment->mNeedsMipmapUpdate = false;
	}
}

void Fbo::markAsDirty()
{
	for( const auto &attachmentPoint : mActiveAttachments ) {
		auto it = mAttachments.find( attachmentPoint );
		if( mAttachments.end() == it ) {
			continue;
		}
		const auto& attachment = it->second;
		// Update per attachment resolve state
		attachment->mNeedsResolve = ( mFormat.mAutoResolve && attachment->mResolve ) ? true : false;
		// Update per attachment mipmap update state
		bool textureHasMipMap = ( attachment->mTexture && attachment->mTexture->hasMipmapping() );
		bool resolveHasMipMap = ( attachment->mResolve && attachment->mResolve->hasMipmapping() );
		// NOTE: Multisample attachments that do not resolve will also no update their mipmap
		attachment->mNeedsMipmapUpdate = ( mFormat.mAutoMipmap && ( textureHasMipMap || ( attachment->mNeedsResolve && resolveHasMipMap ) ) ) ? true : false;
	}
}

void Fbo::bindFramebuffer( GLenum target )
{
	// This in turn will call bindFramebufferImpl; indirection is so that the Context can update its cache of the active Fbo
	gl::context()->bindFramebuffer( shared_from_this(), target );
	markAsDirty();
}

void Fbo::unbindFramebuffer()
{
	context()->unbindFramebuffer();
}

bool Fbo::checkStatus( FboExceptionInvalidSpecification *resultExc )
{
	GLenum status;
	status = (GLenum) glCheckFramebufferStatus( GL_FRAMEBUFFER );
	switch( status ) {
		case GL_FRAMEBUFFER_COMPLETE:
		break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			*resultExc = FboExceptionInvalidSpecification( "Unsupported framebuffer format" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing attachment" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: incomplete attachment" );
		return false;
#if ! defined( CINDER_GL_ES )
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing draw buffer" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: missing read buffer" );
		return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: incomplete multisample" );
		break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: not all attached images are layered" );
		return false;
#else
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: not all attached images have the same number of samples" );
		return false;
#endif
#if defined( CINDER_COCOA_TOUCH ) && ! defined( CINDER_GL_ES_3 )
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer incomplete: not all attached images have the same number of samples" );
		return false;
#endif
		default:
			*resultExc = FboExceptionInvalidSpecification( "Framebuffer invalid: unknown reason" );
		return false;
    }
	
    return true;
}

GLint Fbo::getMaxSamples()
{
#if ! defined( CINDER_GL_ES_2 )
	if( sMaxSamples < 0 ) {
		glGetIntegerv( GL_MAX_SAMPLES, &sMaxSamples);
	}
	
	return sMaxSamples;
#elif defined( CINDER_COCOA_TOUCH )
	if( sMaxSamples < 0 )
		glGetIntegerv( GL_MAX_SAMPLES_APPLE, &sMaxSamples);
	
	return sMaxSamples;
#else
	return 0;
#endif
}

GLint Fbo::getNumSampleCounts( GLenum internalFormat )
{
	if( sNumSampleCounts.end() == sNumSampleCounts.find( internalFormat ) ) {
		GLint numSampleCounts = 0;
		glGetInternalformativ( GL_RENDERBUFFER, internalFormat, GL_NUM_SAMPLE_COUNTS, 1, &numSampleCounts );
		sNumSampleCounts[internalFormat] = numSampleCounts;
	}
	return sNumSampleCounts[internalFormat];
}

GLint Fbo::getMaxAttachments()
{
#if ! defined( CINDER_GL_ES_2 )
	if( sMaxAttachments < 0 ) {
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &sMaxAttachments );
	}
	
	return sMaxAttachments;
#else
	return 1;
#endif
}

void Fbo::setLabel( const std::string &label )
{
	mLabel = label;
	env()->objectLabel( GL_FRAMEBUFFER, mId, (GLsizei)label.size(), label.c_str() );
}

Surface8u Fbo::readPixels8u( const Area &area, GLenum attachmentPoint ) const
{
	// resolve first, before our own bind so that we don't force a resolve unnecessarily
	resolveTextures( attachmentPoint );
	ScopedFramebuffer readScp( GL_FRAMEBUFFER, mId );

	// we need to determine the bounds of the attachment so that we can crop against it and subtract from its height
	Area attachmentBounds = getBounds();
	auto it = mAttachments.find( attachmentPoint );
	if( it != mAttachments.end() ) {
		attachmentBounds = it->second->getBounds();
	}
	else { // the user has attempted to read from an attachment we have no record of
		CI_LOG_W( "Reading from unknown attachment" );
	}
	
	Area clippedArea = area.getClipBy( attachmentBounds );

#if ! defined( CINDER_GL_ES_2 )	
	glReadBuffer( attachmentPoint );
#endif
	Surface8u result( clippedArea.getWidth(), clippedArea.getHeight(), true );
	glReadPixels( clippedArea.x1, attachmentBounds.getHeight() - clippedArea.y2, clippedArea.getWidth(), clippedArea.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, result.getData() );
	
	// glReadPixels returns pixels which are bottom-up
	ip::flipVertical( &result );
	
	return result;
}

#if ! defined( CINDER_GL_ES )
void Fbo::blitTo( const FboRef &dst, const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
{
	ScopedFramebuffer readScp( GL_READ_FRAMEBUFFER, mId );
	ScopedFramebuffer drawScp( GL_DRAW_FRAMEBUFFER, dst->getId() );

	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitToScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask ) const
{
	ScopedFramebuffer readScp( GL_READ_FRAMEBUFFER, mId );
	ScopedFramebuffer drawScp( GL_DRAW_FRAMEBUFFER, 0 );
	
	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}

void Fbo::blitFromScreen( const Area &srcArea, const Area &dstArea, GLenum filter, GLbitfield mask )
{
	ScopedFramebuffer readScp( GL_READ_FRAMEBUFFER, GL_NONE );
	ScopedFramebuffer drawScp( GL_DRAW_FRAMEBUFFER, mId );

	glBlitFramebuffer( srcArea.getX1(), srcArea.getY1(), srcArea.getX2(), srcArea.getY2(), dstArea.getX1(), dstArea.getY1(), dstArea.getX2(), dstArea.getY2(), mask, filter );
}
#endif

std::ostream& operator<<( std::ostream &os, const Fbo &rhs )
{
	os << "\n" << "ID: " << rhs.mId;
	if( rhs.mMultisampleFramebufferId ) {
		os << " Multisample ID: " << rhs.mMultisampleFramebufferId;
	}
	os << std::endl;
	if( ! rhs.mLabel.empty() )
	os << "Label: " << rhs.mLabel << "\n";
	os << "Dims: " << rhs.mWidth << " x " << rhs.mHeight << "\n";
	for( const auto &it : rhs.mAttachments ) {
		const GLenum &attachmentPoint = it.first;
		const auto& attachment = it.second;
		if( attachment->isTexture() ) {
			os << "-Texture Attachment: " << gl::constantToString( attachmentPoint ) << "\n";
			os << *(attachment->getTexture()) << std::endl;
		}
		else if( attachment->isBuffer() ) {
			os << "-Renderbuffer Attachment: " << gl::constantToString( attachmentPoint ) << "\n";
			os << *(attachment->getBuffer()) << std::endl;
		}
	}
	/*
	for( const auto &tex : rhs.mAttachmentsTexture ) {
		os << "-Texture Attachment: " << gl::constantToString( tex.first ) << std::endl;
		os << *tex.second << std::endl;
	}
	for( const auto &ren : rhs.mAttachmentsBuffer ) {
		os << "-Renderbuffer Attachment: " << gl::constantToString( ren.first ) << std::endl;
		os << *ren.second << std::endl;
	}
	*/

/*
	os << "ID: " << rhs.mId;
	if( rhs.mMultisampleFramebufferId )
		os << "  Multisample ID: " << rhs.mMultisampleFramebufferId;
	os << std::endl;
	if( ! rhs.mLabel.empty() )
	os << "  Label: " << rhs.mLabel << std::endl;
	os << "   Dims: " << rhs.mWidth << " x " << rhs.mHeight << std::endl;
	for( const auto &tex : rhs.mAttachmentsTexture ) {
		os << "-Texture Attachment: " << gl::constantToString( tex.first ) << std::endl;
		os << *tex.second << std::endl;
	}
	for( const auto &ren : rhs.mAttachmentsBuffer ) {
		os << "-Renderbuffer Attachment: " << gl::constantToString( ren.first ) << std::endl;
		os << *ren.second << std::endl;
	}
*/

	return os;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FboCubeMap

FboCubeMap::Format::Format()
	: Fbo::Format()
{
	mTextureCubeMapFormat = gl::TextureCubeMap::Format().mipmap();
}

FboCubeMapRef FboCubeMap::create( int32_t faceWidth, int32_t faceHeight, const Format &format )
{
	// create and attach the cubeMap at GL_COLOR_ATTACHMENT0
	auto textureCubeMap = gl::TextureCubeMap::create( faceWidth, faceHeight, format.getTextureCubeMapFormat() );
	auto amendedFormat = format;
	amendedFormat.attachment( GL_COLOR_ATTACHMENT0, textureCubeMap );
	
	return FboCubeMapRef( new FboCubeMap( faceWidth, faceHeight, amendedFormat, textureCubeMap ) );
}

FboCubeMap::FboCubeMap( int32_t faceWidth, int32_t faceHeight, const Format &format, const TextureCubeMapRef &textureCubeMap )
	: Fbo( faceWidth, faceHeight, format ), mTextureCubeMap( textureCubeMap )
{
}

void FboCubeMap::bindFramebufferFace( GLenum faceTarget, GLint level, GLenum target, GLenum attachment )
{
	bindFramebuffer( target );
	glFramebufferTexture2D( target, attachment, faceTarget, mTextureCubeMap->getId(), level );
}

TextureCubeMapRef FboCubeMap::getTextureCubeMap( GLenum attachment )
{
	return std::dynamic_pointer_cast<gl::TextureCubeMap>( getTextureBase( attachment ) );
}

mat4 FboCubeMap::calcViewMatrix( GLenum face, const vec3 &eyePos )
{
	static const vec3 viewDirs[6] = { vec3( 1, 0, 0 ), vec3( -1, 0, 0 ), vec3( 0, 1, 0 ), vec3( 0, -1, 0 ), vec3( 0, 0, 1 ), vec3( 0, 0, -1 ) };
	if( face < GL_TEXTURE_CUBE_MAP_POSITIVE_X || face > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z )
		return mat4();
	
	CameraPersp cam;
	cam.lookAt( eyePos, eyePos + viewDirs[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X] );

	mat4 result;
	// We need to rotate 180deg around Z for non-Y faces
	if( face != GL_TEXTURE_CUBE_MAP_POSITIVE_Y && face != GL_TEXTURE_CUBE_MAP_NEGATIVE_Y )
		result *= glm::rotate( (float)M_PI, vec3( 0, 0, 1 ) );
	
	result *= cam.getViewMatrix();
	
	return result;
}

} } // namespace cinder::gl
