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

FboExceptionInvalidAttachmentFormat::FboExceptionInvalidAttachmentFormat( GLenum attachment, GLenum format )
	: FboException( constantToString( attachment ) + " is not compatible with " + constantToString( format ) )
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::map<GLenum, GLint> Fbo::sNumSampleCounts;
GLint Fbo::sMaxSamples = -1;
GLint Fbo::sMaxAttachments = -1;

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
#if defined( CINDER_MSW ) && ( ! defined( CINDER_GL_ANGLE ) )
	static bool csaaSupported = ( glext_NV_framebuffer_multisample_coverage != 0 );
#else
	static bool csaaSupported = false;
#endif

	glGenRenderbuffers( 1, &mId );

	if( mSamples > Fbo::getNumSampleCounts( internalFormat ) ) {
		mSamples = Fbo::getNumSampleCounts( internalFormat );
	}

	if( ! csaaSupported ) {
		mCoverageSamples = 0;
	}

	gl::ScopedRenderbuffer rbb( GL_RENDERBUFFER, mId );

#if defined( CINDER_GL_HAS_FBO_MULTISAMPLING )
  #if defined( CINDER_MSW )  && ( ! defined( CINDER_GL_ES ) )
	if( mCoverageSamples ) { // create a CSAA buffer
		glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER, mCoverageSamples, mSamples, mInternalFormat, mWidth, mHeight );
	}
	else
  #endif
	{
		if( mSamples ) {
			glRenderbufferStorageMultisample( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
		}
		else {
			glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );
		}
	}
#elif defined( CINDER_GL_ES )
	// this is gross, but GL_RGBA & GL_RGB are not suitable internal formats for Renderbuffers. We know what you meant though.
	if( mInternalFormat == GL_RGBA )
		mInternalFormat = GL_RGBA8_OES;
	else if( mInternalFormat == GL_RGB )
		mInternalFormat = GL_RGB8_OES;
	else if( mInternalFormat == GL_DEPTH_COMPONENT )
		mInternalFormat = GL_DEPTH_COMPONENT24_OES;
		
	if( mSamples ) {
		glRenderbufferStorageMultisample( GL_RENDERBUFFER, mSamples, mInternalFormat, mWidth, mHeight );
	}
	else {
		glRenderbufferStorage( GL_RENDERBUFFER, mInternalFormat, mWidth, mHeight );
	}
#endif	
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
	mColorTextureFormat = getDefaultColorTextureFormat( true );
	mColorTexture = true;
	mColorBuffer = false;
	
	mDepthBufferInternalFormat = getDefaultDepthInternalFormat();
	mDepthBuffer = true;
	mDepthTextureFormat = getDefaultDepthTextureFormat();
	mDepthTexture = false;
	
	mAutoResolve = true;
	mAutoMipmap = true;
	mSamples = 0;
	mCoverageSamples = 0;

	mOverrideTextureSamples = false;

	mStencilTexture = false;
	mStencilBuffer = false;
}

GLint Fbo::Format::getDefaultColorInternalFormat( bool alpha )
{
#if defined( CINDER_GL_ES_2 )
	return GL_RGBA;
#else
	return GL_RGBA8;
#endif
}

GLint Fbo::Format::getDefaultDepthInternalFormat()
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

void Fbo::Format::removeAttachment( GLenum attachmentPoint )
{
	mAttachmentsBuffer.erase( attachmentPoint );
	mAttachmentsMultisampleBuffer.erase( attachmentPoint );	
	mAttachmentsTexture.erase( attachmentPoint );
}
*/

GLenum determineAspectFromFormat( GLenum internalFormat )
{
	GLenum result = GL_COLOR;
	switch( internalFormat ) {
#if defined( CINDER_GL_ES_2 )
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT24_OES:
			result = GL_DEPTH;	
		break;

		case GL_DEPTH24_STENCIL8_OES:
		case GL_DEPTH_STENCIL_OES:
			result = GL_DEPTH_STENCIL;	
		break;
#else
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
			result = GL_DEPTH;
		break;

		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
			result = GL_DEPTH_STENCIL;
		break;
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

Fbo::Format& Fbo::Format::attach( GLenum attachmentPoint, const RenderbufferRef &buffer, const TextureBaseRef &resolve )
{
	GLenum aspect = determineAspectFromFormat( buffer->getInternalFormat() );
	// Error check attachment and internal format
	switch( aspect ) {
		case GL_COLOR: {
			bool isValidAttachment = ( attachmentPoint >= GL_COLOR_ATTACHMENT0 ) && ( attachmentPoint < ( GL_COLOR_ATTACHMENT0 + Fbo::getMaxAttachments() ) );
			if( ! isValidAttachment ) {
				throw FboExceptionInvalidAttachmentFormat( attachmentPoint, buffer->getInternalFormat() );
			}
		}
		break;

		case GL_DEPTH: {
			bool isValidAttachment = ( GL_DEPTH_ATTACHMENT == attachmentPoint );
			if( ! isValidAttachment ) {
				throw FboExceptionInvalidAttachmentFormat( attachmentPoint, buffer->getInternalFormat() );
			}
		}
		break;

		case GL_STENCIL: {
			bool isValidAttachment = ( GL_STENCIL == attachmentPoint );
			if( ! isValidAttachment ) {
				throw FboExceptionInvalidAttachmentFormat( attachmentPoint, buffer->getInternalFormat() );
			}
		}
		break;

		case GL_DEPTH_STENCIL: {
			bool isValidAttachment = ( GL_DEPTH_STENCIL_ATTACHMENT == attachmentPoint );
			if( ! isValidAttachment ) {
				throw FboExceptionInvalidAttachmentFormat( attachmentPoint, buffer->getInternalFormat() );
			}
		}
		break;
	};
	// Create attachment mapping
	Fbo::AttachmentRef attachmentSet = Fbo::AttachmentRef( new Fbo::Attachment() );
	attachmentSet->mBuffer = buffer;
	attachmentSet->mResolve = resolve;
	mAttachments[attachmentPoint] = attachmentSet;
	return *this;
}

Fbo::Format& Fbo::Format::attach( GLenum attachmentPoint, const TextureBaseRef &texture, const TextureBaseRef &resolve )
{
	GLenum aspect = determineAspectFromFormat( texture->getInternalFormat() );
	// Error check attachment and internal format
	switch( aspect ) {
		case GL_COLOR: {
			bool isValidAttachment = ( attachmentPoint >= GL_COLOR_ATTACHMENT0 ) && ( attachmentPoint < ( GL_COLOR_ATTACHMENT0 + Fbo::getMaxAttachments() ) );
			if( ! isValidAttachment ) {
				throw FboExceptionInvalidAttachmentFormat( attachmentPoint, texture->getInternalFormat() );
			}
		}
		break;

		case GL_DEPTH: {
			bool isValidAttachment = ( GL_DEPTH_ATTACHMENT == attachmentPoint );
			if( ! isValidAttachment ) {
				throw FboExceptionInvalidAttachmentFormat( attachmentPoint, texture->getInternalFormat() );
			}
		}
		break;

		case GL_DEPTH_STENCIL: {
			bool isValidAttachment = ( GL_DEPTH_STENCIL_ATTACHMENT == attachmentPoint );
			if( ! isValidAttachment ) {
				throw FboExceptionInvalidAttachmentFormat( attachmentPoint, texture->getInternalFormat() );
			}
		}
		break;
	};
	// Create attachment mapping
	Fbo::AttachmentRef attachmentSet = Fbo::AttachmentRef( new Fbo::Attachment() );
	attachmentSet->mTexture = texture;
	attachmentSet->mResolve = resolve;
	mAttachments[attachmentPoint] = attachmentSet;
	return *this;
}

void Fbo::Format::detach( GLenum attachment )
{
	mAttachments.erase( attachment );
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

FboRef Fbo::create( const Format &format )
{
	return FboRef( new Fbo( format ) );
}

Fbo::Fbo( const Format &format )
	: mId( 0 ), mMultisampleFramebufferId( 0 ), mFormat( format )
{
/*
	if( ! format.mDepthTexture && ! format.mStencilBuffer 
		&& format.mAttachmentsBuffer.empty()
		&& format.mAttachmentsMultisampleBuffer.empty() 
		&& format.mAttachmentsTexture.empty() ) {
		mFormat.mColorTexture = false;
		mFormat.mDepthBuffer = false;
	}
	else {
		ivec2 size = findEffectiveSize( format.mAttachmentsBuffer, format.mAttachmentsTexture );
		mWidth = size.x;
		mHeight = size.y;
	}
*/

	if( format.mAttachments.empty() ) {
		mFormat.mColorTexture = false;
		mFormat.mColorBuffer = false;
		mFormat.mDepthTexture = false;
		mFormat.mDepthBuffer = false;
		mFormat.mStencilTexture = false;
		mFormat.mStencilBuffer = false;
	}
	else {
		ivec2 size; // = findEffectiveSize( format.mAttachmentsBuffer, format.mAttachmentsTexture );
		mWidth = size.x;
		mHeight = size.y;
	}

	init();
	gl::context()->framebufferCreated( this );
}

Fbo::Fbo( int width, int height, const Format &format )
	: mWidth( width ), mHeight( height ), mFormat( format ), mId( 0 ), mMultisampleFramebufferId( 0 )
{
	init();
	gl::context()->framebufferCreated( this );
}

Fbo::~Fbo()
{
	auto ctx = gl::context();
	if( ctx )
		ctx->framebufferDeleted( this );

	if( mId ) {
		glDeleteFramebuffers( 1, &mId );
	}
	if( mMultisampleFramebufferId ) {
		glDeleteFramebuffers( 1, &mMultisampleFramebufferId );
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
		*useMsaa = ( format->mSamples > 0 ) || ( format->mColorTextureFormat.getSamples() > 1 );
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
	
//	if( format->mSamples > Fbo::getMaxSamples() ) {
//		format->mSamples = Fbo::getMaxSamples();
//	}

	// Cap the samples to the max renderbuffer sample size
	if( format->mColorBuffer ) {
		GLint samples = Fbo::getNumSampleCounts( format->mColorBufferInternalFormat );
		format->mSamples = std::min( static_cast<GLint>( format->mSamples ), samples );
	}
	if( format->mDepthBuffer ) {
		GLint samples = Fbo::getNumSampleCounts( format->mDepthBufferInternalFormat );
		format->mSamples = std::min( static_cast<GLint>( format->mSamples ), samples );
	}
}

void Fbo::init()
{
	// Validate attachments
	mAttachments = mFormat.mAttachments;
	mAttachmentCounts = {};
	validateAttachments( mAttachments, mFormat, &mAttachmentCounts );

#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
	bool hasMultisampleAttachments = ( ! mAttachmentCounts.mSampleCounts.empty() ) ||
									 ( mFormat.mColorTexture && ( ( mFormat.getSamples() > 1 ) ||
                                     ( mFormat.mColorTextureFormat.getSamples() > 1 ) ) );
	// Multisample textures and depth renderbuffer do not mix
	if( hasMultisampleAttachments && mFormat.mDepthBuffer ) {
		CI_LOG_W( "Cannot mix renderbuffer depth attachments with multisample color texture attachments. Forcing texture depth attachments instead." );
		mFormat.mDepthBuffer = false;
		mFormat.mDepthTexture = true;
	}
	// Multisample textures and depth renderbuffer do not mix
	if( hasMultisampleAttachments && mFormat.mStencilBuffer ) {
		CI_LOG_W( "Cannot mix renderbuffer stencil attachments with multisample color texture attachments. Forcing texture stencil attachments instead." );
		mFormat.mStencilBuffer = false;
		mFormat.mStencilTexture = true;
	}
#endif

	// allocate the single sample framebuffer
	glGenFramebuffers( 1, &mId );
	
	// determine multisampling settings
	bool useMsaa = false;
	bool useCsaa = false;
	initMultisamplingSettings( &useMsaa, &useCsaa, &mFormat );

	// Make sample counts uniform
	if( ! mAttachmentCounts.mSampleCounts.empty() ) {
		mFormat.setSamples( mAttachmentCounts.mSampleCounts[0] );
		if( mFormat.getSamples() > 1 ) {
			useMsaa = true;
		}
	}

	// allocate the multisample framebuffer
	if( useMsaa || useCsaa ) {
		glGenFramebuffers( 1, &mMultisampleFramebufferId );
	}
    
    // Force buffer on platforms that do not support texture multisample
    if( ( 0 != mMultisampleFramebufferId ) && ( ! gl::env()->supportsTextureMultisample() ) ) {
        // Color
        if( ( ! mAttachmentCounts.hasColor() ) && mFormat.mColorTexture ) {
            mFormat.mColorBuffer = false;
            mFormat.mColorTexture = true;
            CI_LOG_W( "Platform does not support ARB_texture_multisample - using buffer for color attachment." );
        }
		// Depth/stencil combined
		if( ( ! mAttachmentCounts.hasDepthStencil() ) && mFormat.mDepthTexture && mFormat.mStencilTexture ) {
			mFormat.mDepthTexture = false;
			mFormat.mStencilTexture = false;
			mFormat.mDepthBuffer = true;
			mFormat.mStencilBuffer = true;
			CI_LOG_W( "Platform does not support ARB_texture_multisample - using buffer for depth/stencil attachment." );
		}
		else {
			// Depth
			if( mFormat.mDepthTexture ) {
				mFormat.mDepthTexture = false;
				mFormat.mDepthBuffer = true;
				CI_LOG_W( "Platform does not support ARB_texture_multisample - using buffer for depth attachment." );
			}
			// Stencil
			if( mFormat.mStencilTexture ) {
				mFormat.mStencilTexture = false;
				mFormat.mStencilBuffer = true;
				CI_LOG_W( "Platform does not support ARB_texture_multisample - using buffer for stencil attachment." );
			}
		}
    }
    
	// Prepare the attachments
	prepareAttachments( useMsaa || useCsaa );
	// Build initial draw buffers
	for( auto &it : mAttachments ) {
		GLenum attachmentPoint = it.first;
		if( ( attachmentPoint > GL_COLOR_ATTACHMENT0 ) && ( attachmentPoint < ( GL_COLOR_ATTACHMENT0 + Fbo::getMaxAttachments() ) ) ) {
			mDrawBuffers.push_back( attachmentPoint );
		}
	}
	// Attach attachments
	attachAttachments();

/*
	if( useCsaa || useMsaa ) {
		initMultisample( mFormat );
	}

	setDrawBuffers( mId, mAttachmentsBuffer, mAttachmentsTexture );
	if( mMultisampleFramebufferId ) { // using multisampling and setup succeeded
		setDrawBuffers( mMultisampleFramebufferId, mAttachmentsMultisampleBuffer, map<GLenum,TextureBaseRef>() );
	}

	if( ! mAttachmentsBuffer.empty() && ! mAttachmentsTexture.empty() && ! mAttachmentsMultisampleBuffer.empty() ) {
		FboExceptionInvalidSpecification exc;
		if( ! checkStatus( &exc ) ) {
			throw exc;
		}
	}

	mNeedsResolve = false;
	mNeedsMipmapUpdate = false;
*/	

	mLabel = mFormat.mLabel;
	if( ! mLabel.empty() ) {
		env()->objectLabel( GL_FRAMEBUFFER, mId, (GLsizei)mLabel.size(), mLabel.c_str() );
	}
}

void Fbo::validateAttachments( const std::map<GLenum, AttachmentRef> &attachments, const Format &format, struct AttachmentCounts *outCounts )
{
	uint8_t totalAttachmentCount		= 0;
	uint8_t color1DAttachmentCount		= 0;
	uint8_t color2DAttachmentCount		= 0;
	uint8_t color3DAttachmentCount		= 0;
	uint8_t depthAttachmentCount		= 0;
	uint8_t stencilAttachmentCount		= 0;
	uint8_t depthStencilAttachmentCount	= 0;
	uint8_t arrayAttachmentCount		= 0;
	uint16_t minArraySize				= 0;
	uint16_t maxArraySize				= 0;
	std::vector<uint8_t> sampleCounts;

	// Validate attachments
	for( const auto& it : attachments ) {
		GLenum attachmentPoint = it.first;
		GLenum aspect = determineAspectFromAttachmentPoint( attachmentPoint );
		// Increment total attachment count
		if( GL_INVALID_ENUM != aspect ) {
			++totalAttachmentCount;
		}
		// Figure out some properties about the attachments
		const auto& attachment = it.second;
		switch( aspect ) {
			// Color
			case GL_COLOR: {
				const auto& attachment = it.second;
				GLenum target = attachment->mTexture ? attachment->mTexture->getTarget() : GL_INVALID_ENUM;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
				switch( target ) {
					case GL_TEXTURE_1D: ++color1DAttachmentCount; break;
					case GL_TEXTURE_2D_MULTISAMPLE:
					case GL_TEXTURE_2D: ++color2DAttachmentCount; break;
					case GL_TEXTURE_3D: ++color3DAttachmentCount; break;
					case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
					case GL_TEXTURE_2D_ARRAY: {
						++color2DAttachmentCount; 
						++arrayAttachmentCount;
						uint16_t depth = attachment->mTexture->getDepth();
						minArraySize = ( 0 == minArraySize ) ? depth : std::min( minArraySize, depth );
						maxArraySize = ( 0 == maxArraySize ) ? depth : std::max( maxArraySize, depth );
					}
					break;
				}
				// Sample count
				if( ( GL_TEXTURE_2D_MULTISAMPLE == target ) || ( GL_TEXTURE_2D_MULTISAMPLE_ARRAY ) ) {
					sampleCounts.push_back( static_cast<uint32_t>( attachment->mTexture->getSamples() ) );
				}
#else 
				switch( target ) {
					case GL_TEXTURE_1D: ++color1DAttachmentCount; break;
					case GL_TEXTURE_2D: ++color2DAttachmentCount; break;
					case GL_TEXTURE_3D: ++color3DAttachmentCount; break;
					case GL_TEXTURE_2D_ARRAY: {
						++color2DAttachmentCount; 
						++arrayAttachmentCount;
						uint32_t depth = attachment->mTexture->getDepth();
						minArraySize = ( 0 == minArraySize ) ? depth : std::min( minArraySize, depth );
						maxArraySize = ( 0 == maxArraySize ) ? depth : std::max( maxArraySize, depth );
					}
					break;
				}
#endif
			}
			break;
			// Depth stencil
			case GL_DEPTH: {
				++depthAttachmentCount;
			}
			break;
			
			case GL_STENCIL: {
			}
				++stencilAttachmentCount;
			break;
			
			case GL_DEPTH_STENCIL: {
				++depthStencilAttachmentCount;
			}
			break;
		}
	}

	// Cannot mix array and non-array attachments
	if( ( arrayAttachmentCount > 0 ) && ( arrayAttachmentCount != color2DAttachmentCount ) ) {
		throw FboException( "Cannot mix array and non-array attachments" );
	}

#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
	// Sample counts for all multisample attachments must be the same
	if( ( ! sampleCounts.empty() ) && ( sampleCounts.end() == std::adjacent_find( std::begin( sampleCounts ), std::end( sampleCounts ), std::not_equal_to<uint32_t>() ) ) ) {
		throw FboException( "Sample counts for all multisample attachments must be the same" );
	}
#endif

	// GL_TEXTURE_3D targets do not support depth formats
	if( ( format.mDepthTexture || format.mDepthBuffer ) && ( color3DAttachmentCount > 0 ) ) {
		throw FboException( "GL_TEXTURE_3D targets do not support depth formats" );
	}

	// GL_TEXTURE_3D does not support stencil formats
	if( format.mDepthTexture && ( color3DAttachmentCount > 0 ) ) {
		throw FboException( "GL_TEXTURE_3D targets do not support stencil formats" );
	}

	outCounts->mTotalAttachmentCount		= totalAttachmentCount;
	outCounts->mColor1DAttachmentCount		= color1DAttachmentCount;
	outCounts->mColor2DAttachmentCount		= color2DAttachmentCount;
	outCounts->mColor3DAttachmentCount		= color3DAttachmentCount;
	outCounts->mDepthAttachmentCount		= depthAttachmentCount;
	outCounts->mStencilAttachmentCount		= stencilAttachmentCount;
	outCounts->mDepthStencilAttachmentCount	= depthStencilAttachmentCount;
	outCounts->mArrayAttachmentCount		= arrayAttachmentCount;
	outCounts->mMinArraySize				= minArraySize;
	outCounts->mMaxArraySize				= maxArraySize;
	outCounts->mSampleCounts				= sampleCounts;
}

// Iterate the Format's requested attachments and create any we don't already have attachments for
#if defined( CINDER_GL_ES_2 )
/*
void Fbo::prepareAttachments( const Fbo::Format &format, bool multisampling )
{
	mAttachmentsBuffer = format.mAttachmentsBuffer;
	mAttachmentsTexture = format.mAttachmentsTexture;

	// Create the default color attachment if there's not already something on GL_COLOR_ATTACHMENT0
	bool preexistingColorAttachment = mAttachmentsTexture.count( GL_COLOR_ATTACHMENT0 ) || mAttachmentsBuffer.count( GL_COLOR_ATTACHMENT0 );
	if( format.mColorTexture && ( ! preexistingColorAttachment ) ) {
		mAttachmentsTexture[GL_COLOR_ATTACHMENT0] = Texture::create( mWidth, mHeight, format.mColorTextureFormat );
	}
	
	// Create the default depth(+stencil) attachment if there's not already something on GL_DEPTH_ATTACHMENT || GL_DEPTH_STENCIL_ATTACHMENT
	bool preexistingDepthAttachment		= mAttachmentsTexture.count( GL_DEPTH_ATTACHMENT ) || mAttachmentsBuffer.count( GL_DEPTH_ATTACHMENT );
	bool layeredColorAttachment			= false;
	bool isDepthAttachmentCompatible	= true;

	if( ! preexistingDepthAttachment && isDepthAttachmentCompatible ) {
		// If the color attachment is layered all attachment need to be layered for the fbo to be complete. As layered RenderBuffers don't exist we force the use of a depth texture
		if( format.mDepthTexture || ( ( format.mDepthBuffer || format.mDepthTexture ) && layeredColorAttachment ) ) { // depth texture or depth_stencil texture
			if( format.mStencilBuffer ) {
				GLint internalFormat;
				GLenum pixelDataType;
				Format::getDepthStencilFormats( format.mDepthBufferInternalFormat, &internalFormat, &pixelDataType );
				Texture2dRef depthStencilTexture = Texture2d::create( mWidth, mHeight, Texture2d::Format( format.mDepthTextureFormat ).internalFormat( internalFormat ).dataType( pixelDataType ) );
				mAttachmentsTexture[GL_DEPTH_ATTACHMENT] = depthStencilTexture;
				mAttachmentsTexture[GL_STENCIL_ATTACHMENT] = depthStencilTexture;
			}
			else {
				mAttachmentsTexture[GL_DEPTH_ATTACHMENT] = Texture2d::create( mWidth, mHeight, format.mDepthTextureFormat );
			}
		}
		else if( format.mDepthBuffer ) { // depth buffer or depth_stencil buffer	
			if( format.mStencilBuffer ) {
				GLint internalFormat;
				GLenum pixelDataType;
				Format::getDepthStencilFormats( format.mDepthBufferInternalFormat, &internalFormat, &pixelDataType );
				RenderbufferRef depthStencilBuffer = Renderbuffer::create( mWidth, mHeight, internalFormat );
				mAttachmentsBuffer[GL_DEPTH_ATTACHMENT] = depthStencilBuffer;
				mAttachmentsBuffer[GL_STENCIL_ATTACHMENT] = depthStencilBuffer;
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
	}
	else if( ! isDepthAttachmentCompatible && format.mDepthTexture ) {
		CI_LOG_W( "Depth Texture not compatible with Texture3d. Use Texture2d Array instead." );
	}
	else if( ! isDepthAttachmentCompatible && format.mStencilBuffer ) {
		CI_LOG_W( "Stencil Buffer only not compatible with Texture3d. Use a DEPTH_STENCIL format instead." );
	}
}
*/
#else 
void Fbo::prepareAttachments( bool multisample )
{
	bool hasColorAttachment = ( ( mAttachmentCounts.mColor1DAttachmentCount > 0 ) || ( mAttachmentCounts.mColor2DAttachmentCount > 0 ) || ( mAttachmentCounts.mColor3DAttachmentCount > 0 ) );
	bool hasDepthAttachment = ( mAttachmentCounts.mDepthAttachmentCount > 0 );

	// Create color attachment
	if( ! hasColorAttachment ) {
		// Create attachment using texture
		if( mFormat.mColorTexture ) {
			TextureBaseRef texture;
			TextureBaseRef resolve;
			auto colorFormat = mFormat.mColorTextureFormat;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
			if( multisample && ( mFormat.mSamples > 1 ) && ( 1 == colorFormat.getSamples() ) ) {
				colorFormat.setSamples( mFormat.mSamples, colorFormat.hasFixedSampleLocations() );
			}
#endif
			texture = Texture::create( mWidth, mHeight, colorFormat );
			if( texture->getSamples() > 1 ) {
				auto resolveFormat = mFormat.mColorTextureFormat;
				resolveFormat.setSamples( 1 );
				resolve = Texture::create( mWidth, mHeight, resolveFormat );
			}
			privateAttach( GL_COLOR_ATTACHMENT0, texture, nullptr, resolve );
			// Update array attachment count
			GLenum target = texture->getTarget();
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
			if( ( GL_TEXTURE_2D_ARRAY == target ) || ( GL_TEXTURE_2D_MULTISAMPLE_ARRAY == target ) ) {
				++(mAttachmentCounts.mArrayAttachmentCount);
			}
#else
			if( GL_TEXTURE_2D_ARRAY == target ) {
				++(mAttachmentCounts.mArrayAttachmentCount);
			}
#endif
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
			privateAttach( GL_COLOR_ATTACHMENT0, nullptr, buffer, resolve );
		}
	}

	// Create depth/stencil attachment
	if( ( ! hasDepthAttachment ) && ( mFormat.mDepthTexture || mFormat.mDepthBuffer || mFormat.mStencilBuffer ) ) {
		// Force a texture if depth/stencil is requested with array attachments - RenderBuffers do not have array support.
		if( ( mAttachmentCounts.mArrayAttachmentCount > 0 ) && ( mFormat.mDepthTexture || mFormat.mDepthBuffer ) ) {
			// Find the first color attachment
			auto it = std::find_if( std::begin( mAttachments ), std::end( mAttachments ),
				[]( const std::pair<GLenum, Fbo::AttachmentRef>& elem ) -> bool {
					GLenum attachmentPoint = elem.first;
					bool isColorAttachment = ( ( attachmentPoint >= GL_COLOR_ATTACHMENT0 ) && ( attachmentPoint < ( GL_COLOR_ATTACHMENT0 + Fbo::getMaxAttachments() ) ) );
					return isColorAttachment;
				}
			);
			Fbo::AttachmentRef colorAttachment =  ( it != mAttachments.end() ) ? it->second : Fbo::AttachmentRef();
			// If an attachment is not found here, something has gone horribly wrong!
			if( ( ! colorAttachment ) || ( colorAttachment && ( ! colorAttachment->mTexture ) ) ) {
				throw FboException( "Couldn't find color texture array attachment to configure depth attachment" );
			}
			// Create depth/stencil buffer based on the found color attachment
			GLint depth = colorAttachment->mTexture->getDepth();
			GLenum target = colorAttachment->mTexture->getTarget();
			if( mFormat.mStencilBuffer ) {
				// Get internal format
				GLint internalFormat;
				GLenum pixelDataType;
				Format::getDepthStencilFormats( mFormat.mDepthBufferInternalFormat, &internalFormat, &pixelDataType );
				// Create texture
				TextureBaseRef texture = Texture3d::create( mWidth, mHeight, depth, Texture3d::Format( mFormat.mDepthTextureFormat ).internalFormat( internalFormat ).target( target ) );
				privateAttach( GL_DEPTH_STENCIL_ATTACHMENT, texture, nullptr, nullptr );
			}
			else {
				TextureBaseRef texture = Texture3d::create( mWidth, mHeight, depth, Texture3d::Format( mFormat.mDepthTextureFormat ).target( target ) );
				privateAttach( GL_DEPTH_ATTACHMENT, texture, nullptr, nullptr );
			}
		}
		else {
			// Combined depth and stencil texture
			if( ( mFormat.mDepthTexture && mFormat.mStencilTexture ) || ( mFormat.mDepthTexture && mFormat.mStencilBuffer ) || ( mFormat.mDepthBuffer && mFormat.mStencilTexture ) ) {
				auto depthStencilFormat = mFormat.mDepthTextureFormat;
				depthStencilFormat.setSamples( mFormat.mSamples );
				TextureBaseRef texture = Texture2d::create( mWidth, mHeight, depthStencilFormat );
				TextureBaseRef resolve;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
				if( depthStencilFormat.getSamples() > 1 ) {
					auto resolveFormat = depthStencilFormat;
					resolveFormat.setSamples( 1 );
					resolve = Texture2d::create( mWidth, mHeight, resolveFormat );
				}
#endif
				privateAttach( GL_DEPTH_STENCIL_ATTACHMENT, texture, nullptr, resolve );
			}
			// Combined depth and stencil buffer
			else if( mFormat.mDepthBuffer && mFormat.mStencilBuffer ) {
				GLint internalFormat;
				GLenum pixelDataType;
				Format::getDepthStencilFormats( mFormat.mDepthBufferInternalFormat, &internalFormat, &pixelDataType );
				RenderbufferRef buffer = Renderbuffer::create( mWidth, mHeight, internalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
				TextureBaseRef resolve = Texture2d::create( mWidth, mHeight, Texture2d::Format().internalFormat( internalFormat ).samples( 1 ) );
				privateAttach( GL_DEPTH_STENCIL_ATTACHMENT, nullptr, buffer, resolve );
			} 
			else {
				// Depth texture
				if( mFormat.mDepthTexture ) {
					auto depthFormat = mFormat.mDepthTextureFormat;
					depthFormat.setSamples( mFormat.mSamples );
					TextureBaseRef texture = Texture2d::create( mWidth, mHeight, depthFormat );
					TextureBaseRef resolve;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
					if( depthFormat.getSamples() > 1 ) {
						auto resolveFormat = depthFormat;
						resolveFormat.setSamples( 1 );
						resolve = Texture2d::create( mWidth, mHeight, resolveFormat );
					}
#endif
					privateAttach( GL_DEPTH_ATTACHMENT, texture, nullptr, resolve );
				}
				// Depth buffer
				if( mFormat.mDepthBuffer ) {
					RenderbufferRef buffer = Renderbuffer::create( mWidth, mHeight, mFormat.mDepthBufferInternalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
					TextureBaseRef resolve = Texture2d::create( mWidth, mHeight, Texture2d::Format().internalFormat( mFormat.mDepthBufferInternalFormat ).samples( 1 ) );
					privateAttach( GL_DEPTH_ATTACHMENT, nullptr, buffer, resolve );
				}
				// Stencil texture
				if( mFormat.mStencilTexture ) {
					auto stencilFormat = Texture2d::Format().internalFormat( GL_STENCIL_INDEX8 );
					stencilFormat.setSamples( mFormat.mSamples );
					TextureBaseRef texture = Texture2d::create( mWidth, mHeight, stencilFormat );
					TextureBaseRef resolve;
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
					if( stencilFormat.getSamples() > 1 ) {
						auto resolveFormat = stencilFormat;
						resolveFormat.setSamples( 1 );
						resolve = Texture2d::create( mWidth, mHeight, resolveFormat );
					}
#endif
					privateAttach( GL_STENCIL_ATTACHMENT, texture, nullptr, resolve );
				}
				// Stencil buffer
				if( mFormat.mStencilBuffer ) {
					GLint internalFormat = GL_STENCIL_INDEX8;
					RenderbufferRef buffer = Renderbuffer::create( mWidth, mHeight, internalFormat, mFormat.mSamples, mFormat.mCoverageSamples );
					TextureBaseRef resolve = Texture2d::create( mWidth, mHeight, Texture2d::Format().internalFormat( internalFormat ).samples( 1 ) );
					privateAttach( GL_STENCIL_ATTACHMENT, nullptr, buffer, resolve );
				}
			}
		}
	}
}
#endif

#if defined( CINDER_GL_ES )
void Fbo::attachAttachments()
{
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
					framebufferTexture2d( GL_FRAMEBUFFER, attachmentPoint, target, id, 0 );
				}
				else {
					framebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
				}
			}
			else if( attachment->mBuffer ) {
				GLuint id = attachment->mBuffer->getId();
				framebufferRenderbuffer( GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, id );
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
			framebufferTexture( GL_FRAMEBUFFER, attachmentPoint, id, 0 );
		}
	}
}
#endif 

/*
void Fbo::attachAttachments()
{
	// attach Renderbuffers
	for( auto &bufferAttachment : mAttachmentsBuffer ) {
		framebufferRenderbuffer( GL_FRAMEBUFFER, bufferAttachment.first, GL_RENDERBUFFER, bufferAttachment.second->getId() );
	}
	
	// attach Textures
	for( auto &textureAttachment : mAttachmentsTexture ) {
		auto textureTarget = textureAttachment.second->getTarget();
#if ! defined( CINDER_GL_ES )
		if( textureTarget == GL_TEXTURE_CUBE_MAP ) {
			textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
			framebufferTexture2d( GL_FRAMEBUFFER, textureAttachment.first, textureTarget, textureAttachment.second->getId(), 0 );
		}
		else {
			framebufferTexture( GL_FRAMEBUFFER, textureAttachment.first, textureAttachment.second->getId(), 0 );
		}
#else
		if( textureTarget == GL_TEXTURE_CUBE_MAP )
			textureTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		framebufferTexture2d( GL_FRAMEBUFFER, textureAttachment.first, textureTarget, textureAttachment.second->getId(), 0 );
#endif
	}	
}
*/

void Fbo::privateAttach( GLenum attachmentPoint, const TextureBaseRef &texture, const RenderbufferRef &buffer, const TextureBaseRef &resolve )
{
	if( texture && buffer ) {
		throw FboException( "Attachment must be either texture or buffer but not both" );
	}

	// Create attachment
	Fbo::AttachmentRef attachment = Fbo::AttachmentRef( new Fbo::Attachment() );
	attachment->mTexture = texture;
	attachment->mBuffer = buffer;
	attachment->mResolve = resolve;

	// If texture or buffer is multisample and a resolve texture is not specified - create one.
#if defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
	if( attachment->mTexture && ( GL_TEXTURE_2D_MULTISAMPLE == attachment->mTexture->getTarget() ) && ( ! attachment->mResolve ) ) {
		if( typeid( *attachment->mTexture ) == typeid( Texture2d ) ) {
			auto msTexture = std::dynamic_pointer_cast<Texture2d>( attachment->mTexture );
			if( msTexture ) {
				auto resolveFormat = msTexture->getFormat();
				resolveFormat.setSamples( 1 );
				attachment->mResolve = Texture2d::create( msTexture->getWidth(), msTexture->getHeight(), resolveFormat );
			}
		}
	}
#endif 

#if defined( CINDER_GL_HAS_FBO_MULTISAMPLING ) || defined( CINDER_GL_HAS_TEXTURE_MULTISAMPLE )
	if( attachment->mBuffer && ( attachment->mBuffer->getSamples() > 1 ) && ( ! attachment->mResolve ) ) {
	}
#endif

	// Map attachment
	mAttachments[attachmentPoint] = attachment;
}

// call glDrawBuffers against all color attachments
void Fbo::setDrawBuffers( GLuint fbId, const map<GLenum,RenderbufferRef> &attachmentsBuffer, const map<GLenum,TextureBaseRef> &attachmentsTexture )
{
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
	/*else {
		GLenum none = GL_NONE;
		glDrawBuffers( 1, &none );
	}*/
#endif
}

/*
void Fbo::initMultisample( const Format &format )
{
	glGenFramebuffers( 1, &mMultisampleFramebufferId );
	ScopedFramebuffer fbScp( GL_FRAMEBUFFER, mMultisampleFramebufferId );

	mAttachmentsMultisampleBuffer = format.mAttachmentsMultisampleBuffer;

	// create mirror Multisample Renderbuffers for any Buffer attachments in the primary FBO
	for( const auto &bufferAttachment : mAttachmentsBuffer ) {
		auto existing = mAttachmentsMultisampleBuffer.find( bufferAttachment.first );
		// if there's no existing multisample buffer attachment or it's null
		if( existing == mAttachmentsMultisampleBuffer.end() || ( ! existing->second ) )
			mAttachmentsMultisampleBuffer[bufferAttachment.first] = Renderbuffer::create( mWidth, mHeight, bufferAttachment.second->getInternalFormat(), format.mSamples, format.mCoverageSamples );
	}

	// create mirror Multisample Renderbuffers for any Texture attachments in the primary FBO
	for( auto &bufferAttachment : mAttachmentsTexture ) {
		auto existing = mAttachmentsMultisampleBuffer.find( bufferAttachment.first );
		// if there's no existing multisample buffer attachment or it's null
		if( existing == mAttachmentsMultisampleBuffer.end() || ( ! existing->second ) )
			mAttachmentsMultisampleBuffer[bufferAttachment.first] = Renderbuffer::create( mWidth, mHeight, bufferAttachment.second->getInternalFormat(), format.mSamples, format.mCoverageSamples );
	}

	// attach MultisampleRenderbuffers
	for( auto &bufferAttachment : mAttachmentsMultisampleBuffer )
		framebufferRenderbuffer( GL_FRAMEBUFFER, bufferAttachment.first, GL_RENDERBUFFER, bufferAttachment.second->getId() );

	FboExceptionInvalidSpecification ignoredException;
	if( ! checkStatus( &ignoredException ) ) { // failure
		CI_LOG_W( "Failed to initialize FBO multisampling" );
		mAttachmentsMultisampleBuffer.clear();
		glDeleteFramebuffers( 1, &mMultisampleFramebufferId );
		mMultisampleFramebufferId = 0;
	}
}
*/

Texture2dRef Fbo::getColorTexture( GLenum attachmentPoint )
{
	Texture2dRef result;

	auto it = mAttachments.find( attachmentPoint );
	if( it != mAttachments.end() ) {
		auto& attachment = it->second;
		if( attachment->mResolve ) {
			if( mFormat.mAutoResolve ) {
				resolveTextures( attachmentPoint );
				// Only update if there's a resolve	
				if( mFormat.mAutoMipmap ) {
					updateMipmaps( attachmentPoint );
				}
			}
			result = std::static_pointer_cast<Texture2d>( attachment->mResolve );
		}
		else {
			if( mFormat.mAutoMipmap ) {
				updateMipmaps( attachmentPoint );
			}
			result = std::static_pointer_cast<Texture2d>( attachment->mTexture );
		}
	}

	return result;

/*
	auto attachedTextureIt = mAttachmentsTexture.find( GL_COLOR_ATTACHMENT0 );
	if( attachedTextureIt != mAttachmentsTexture.end() && ( typeid(*attachedTextureIt->second) == typeid(Texture2d) ) ) {
		resolveTextures();
		updateMipmaps( GL_COLOR_ATTACHMENT0 );
		return static_pointer_cast<Texture2d>( attachedTextureIt->second );
	}
	else
		return Texture2dRef();
*/
}

Texture2dRef Fbo::getDepthTexture()
{
	return Texture2dRef();

/*
	TextureBaseRef result;
	
	// search for a depth attachment
	auto attachedTextureIt = mAttachmentsTexture.find( GL_DEPTH_ATTACHMENT );
	if( attachedTextureIt != mAttachmentsTexture.end() ) {
		result = attachedTextureIt->second;
	}
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
	
	return Texture2dRef();
*/
}

Texture2dRef Fbo::getTexture2d( GLenum attachment )
{
	return dynamic_pointer_cast<Texture2d>( getTextureBase( attachment ) );
}

TextureBaseRef Fbo::getTextureBase( GLenum attachment )
{
	return TextureBaseRef();

/*
	if( ( (attachment < GL_COLOR_ATTACHMENT0) || (attachment > MAX_COLOR_ATTACHMENT) ) && (attachment != GL_DEPTH_ATTACHMENT)
#if ! defined( CINDER_GL_ES_2 )
		&& (attachment != GL_DEPTH_STENCIL_ATTACHMENT) )
#else
	)
#endif
	{
		CI_LOG_W( "Illegal constant for texture attachment: " << gl::constantToString( attachment ) );
	}
	
	auto attachedTextureIt = mAttachmentsTexture.find( attachment );
	if( attachedTextureIt != mAttachmentsTexture.end() ) {
		resolveTextures();
		updateMipmaps( attachment );
		return attachedTextureIt->second;
	}
	else
		return TextureBaseRef();
*/
}

void Fbo::bindTexture( int textureUnit, GLenum attachment )
{
	auto tex = getTextureBase( attachment );
	if( tex ) {
		tex->bind( textureUnit );
	}
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
		auto ctx = context();
		ctx->pushFramebuffer( GL_DRAW_FRAMEBUFFER, mId );
		ctx->pushFramebuffer( GL_READ_FRAMEBUFFER, mMultisampleFramebufferId );

		std::map<GLenum, Fbo::AttachmentRef> attachments;
		if( Fbo::ALL_ATTACHMENTS == attachmentPoint ) {
			for( auto &attachmentPoint : mDrawBuffers ) {
				auto it = mAttachments.find( attachmentPoint );
				attachments[attachmentPoint] = it->second;
			}
		}
		else {
			auto it = mAttachments.find( attachmentPoint );
			if( it != mAttachments.end() ) {
				attachments[it->first] = it->second;
			}
		}

		// Process attachments
		for( auto& it : attachments ) {
			GLenum attachmentPoint = it.first;
			auto& attachment = it.second;

			glDrawBuffers( 1, &attachmentPoint );
			glReadBuffer( attachmentPoint );
#if ! defined( CINDER_GL_ANGLE )
			// ANGLE appears to error when requested to resolve a depth buffer
			glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
#else
			glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST );
#endif
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
		ctx->bindFramebuffer( GL_FRAMEBUFFER, mMultisampleFramebufferId );
		glDrawBuffers( static_cast<GLsizei>( mDrawBuffers.size() ), mDrawBuffers.data() );

		ctx->popFramebuffer( GL_DRAW_FRAMEBUFFER );
		ctx->popFramebuffer( GL_READ_FRAMEBUFFER );	
	}
#endif

/*
	if( ! mNeedsResolve )
		return;

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
	// if this FBO is multisampled, resolve it, so it can be displayed
	if( mMultisampleFramebufferId ) {
		auto ctx = context();

		ctx->pushFramebuffer( GL_DRAW_FRAMEBUFFER, mId );
		ctx->pushFramebuffer( GL_READ_FRAMEBUFFER, mMultisampleFramebufferId );

		vector<GLenum> drawBuffers;
		for( GLenum c = GL_COLOR_ATTACHMENT0; c <= MAX_COLOR_ATTACHMENT; ++c ) {
            auto colorAttachmentIt = mAttachmentsTexture.find( c );
            if( colorAttachmentIt != mAttachmentsTexture.end() ) {
                glDrawBuffers( 1, &colorAttachmentIt->first );
                glReadBuffer( colorAttachmentIt->first );
#if ! defined( CINDER_GL_ANGLE )
				// ANGLE appears to error when requested to resolve a depth buffer
				glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
#else
				glBlitFramebuffer( 0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST );
#endif
				drawBuffers.push_back( colorAttachmentIt->first );
			}
		}

		// restore the draw buffers to the default for the antialiased (non-resolve) framebuffer
		ctx->bindFramebuffer( GL_FRAMEBUFFER, mMultisampleFramebufferId );
		glDrawBuffers( (GLsizei)drawBuffers.size(), &drawBuffers[0] );
		
		ctx->popFramebuffer( GL_DRAW_FRAMEBUFFER );
		ctx->popFramebuffer( GL_READ_FRAMEBUFFER );		
	}
#endif

	mNeedsResolve = false;
*/
}


void Fbo::attach( GLenum attachmentPoint, const RenderbufferRef &buffer, const TextureBaseRef &resolve )
{
/*
	if( ( (attachment < GL_COLOR_ATTACHMENT0) || (attachment > MAX_COLOR_ATTACHMENT) ) && (attachment != GL_DEPTH_ATTACHMENT)
#if ! defined( CINDER_GL_ES_2 )
		&& (attachment != GL_DEPTH_STENCIL_ATTACHMENT) )
#else
	)
#endif
	{
		CI_LOG_W( "Illegal constant for texture attachment: " << gl::constantToString( attachment ) );
	}

	ScopedFramebuffer fbScp( GL_DRAW_FRAMEBUFFER, mId );
	gl::framebufferRenderbuffer( renderbuffer, attachment );
	checkStatus();
	
	// update fbo attachment slots
	mAttachmentsBuffer[attachment] = renderbuffer;
	mAttachmentsTexture.erase( attachment );
	if( mFormat.mSamples > 0 ) {
		//mAttachmentsMultisampleBuffer[attachment] = multisampleBuffer;
	}
	else {
		mAttachmentsMultisampleBuffer.erase( attachment );
	}
	
	// update fbo size
	ivec2 size = findEffectiveSize( mAttachmentsBuffer, mAttachmentsTexture );
	mWidth = size.x;
	mHeight = size.y;
*/
}

void Fbo::attach( GLenum attachmentPoint, const TextureBaseRef &texture, GLint level )
{
/*
	if( ( (attachment < GL_COLOR_ATTACHMENT0) || (attachment > MAX_COLOR_ATTACHMENT) ) && (attachment != GL_DEPTH_ATTACHMENT)
#if ! defined( CINDER_GL_ES_2 )
		&& (attachment != GL_DEPTH_STENCIL_ATTACHMENT) )
#else
	)
#endif
	{
		CI_LOG_W( "Illegal constant for texture attachment: " << gl::constantToString( attachment ) );
	}

	ScopedFramebuffer fbScp( GL_DRAW_FRAMEBUFFER, mId );
	gl::framebufferTexture( texture, attachment, level );
	checkStatus();
	
	// update fbo attachment slots
	mAttachmentsTexture[attachment] = texture;
	mAttachmentsBuffer.erase( attachment );
	if( mFormat.mSamples > 0 ) {
		//mAttachmentsMultisampleBuffer[attachment] = multisampleBuffer;
	}
	else {
		mAttachmentsMultisampleBuffer.erase( attachment );
	}
	
	// update fbo size
	ivec2 size = findEffectiveSize( mAttachmentsBuffer, mAttachmentsTexture );
	mWidth = size.x;
	mHeight = size.y;
*/
}

void Fbo::attach( GLenum attachmentPoint, const TextureBaseRef &texture, const TextureBaseRef &resolve )
{
}

void Fbo::updateMipmaps( GLenum attachmentPoint ) const
{
	auto it = mAttachments.find( attachmentPoint );
	if( it == mAttachments.end() ) {
		return;
	}

	auto &attachment = it->second;
	if( attachment->mNeedsMipmapUpdate ) {
		auto &texture = attachment->mResolve ? attachment->mResolve : attachment->mTexture;
		ScopedTextureBind textureBind( texture );
		glGenerateMipmap( texture->getTarget() );
		attachment->mNeedsMipmapUpdate = false;
	}

/*
	if( mNeedsMipmapUpdate ) {
		// Update bit maps if it hasn't been explicitly disabled
		if( mFormat.mAutoMipmap ) {
			auto textureIt = mAttachmentsTexture.find( attachment );
			if( textureIt != mAttachmentsTexture.end() ) {
				ScopedTextureBind textureBind( textureIt->second );
				glGenerateMipmap( textureIt->second->getTarget() );
			}
		}
		// Clear the flag to maintain good state
		mNeedsMipmapUpdate = false;
	}
*/

/*
	if( ! mNeedsMipmapUpdate ) {
		return;
	}
	else {
		auto textureIt = mAttachmentsTexture.find( attachment );
		if( textureIt != mAttachmentsTexture.end() ) {
			ScopedTextureBind textureBind( textureIt->second );
			glGenerateMipmap( textureIt->second->getTarget() );
		}
	}

	mNeedsMipmapUpdate = false;
*/
}

// The effective size of the FBO is the intersection of all of the sizes of the bound images (ie: the smallest in each dimension). 
ivec2 Fbo::findEffectiveSize( const std::map<GLenum,RenderbufferRef> &buffers, const std::map<GLenum,TextureBaseRef> &textures ) const
{
	ivec2 minSize( 0, 0 );
	for( const auto &buffer : buffers ) {
		ivec2 bufferSize( buffer.second->getSize() );
		if( minSize.x == 0 )
			minSize.x = bufferSize.x;
		if( minSize.y == 0 )
			minSize.y = bufferSize.y;
		minSize = glm::min( minSize, bufferSize );
	}
	
	for( const auto &texture : textures ) {
		ivec2 textureSize( texture.second->getWidth(), texture.second->getHeight() );
		if( minSize.x == 0 )
			minSize.x = textureSize.x;
		if( minSize.y == 0 )
			minSize.y = textureSize.y;
		minSize = glm::min( minSize, textureSize );
	}

	return minSize;
}

ivec2 Fbo::findEffectiveSize( const std::map<GLenum, Fbo::AttachmentRef>& colorAttachments, const Fbo::AttachmentRef &depthAttachment, const Fbo::AttachmentRef &stencilAttachment )
{
	ivec2 minSize = ivec2( 0, 0 );

	auto getAttachmentSize = []( const Fbo::AttachmentRef &attachmentSet ) -> ivec2 {
		ivec2 result = ivec2( 0, 0 );
		if( attachmentSet->mTexture ) {
			result = ivec2( attachmentSet->mTexture->getWidth(),  attachmentSet->mTexture->getHeight() );
		}
		else if( attachmentSet->mBuffer ) {
			result = attachmentSet->mBuffer->getSize();
		}
		return result;
	};

	for( const auto& it : colorAttachments ) {
		ivec2 size = getAttachmentSize( it.second );
		if( 0 == minSize.x ) {
			minSize.x = size.x;
		}
		if( 0 == minSize.y ) {
			minSize.y = size.y;
		}
		minSize = glm::min( minSize, size );
	}

	if( depthAttachment ) {
		ivec2 size = getAttachmentSize( depthAttachment );
		if( 0 == minSize.x ) {
			minSize.x = size.x;
		}
		if( 0 == minSize.y ) {
			minSize.y = size.y;
		}
		minSize = glm::min( minSize, size );
	}

	if( stencilAttachment ) {
		ivec2 size = getAttachmentSize( stencilAttachment );
		if( 0 == minSize.x ) {
			minSize.x = size.x;
		}
		if( 0 == minSize.y ) {
			minSize.y = size.y;
		}
		minSize = glm::min( minSize, size );
	}

	return minSize;
}

void Fbo::markAsDirty()
{
	for( const auto &attachmentPoint : mDrawBuffers ) {
		const auto& attachment = mAttachments[attachmentPoint];
		// Update per attachment resolve state
		attachment->mNeedsResolve = ( mFormat.mAutoResolve && attachment->mResolve ) ? true : false;
		// Update per attachment mipmap update state
		bool textureHasMipMap = ( attachment->mTexture && attachment->mTexture->hasMipmapping() );
		bool resolveHasMipMap = ( attachment->mResolve && attachment->mResolve->hasMipmapping() );
		// NOTE: Multisample attachments that do not resolve will also no update their mipmap
		attachment->mNeedsMipmapUpdate = ( mFormat.mAutoMipmap && ( textureHasMipMap || ( attachment->mNeedsResolve && resolveHasMipMap ) ) ) ? true : false;
	}

	/*
	if( mMultisampleFramebufferId ) {
		mNeedsResolve = true;
	}

	for( const auto &it : mAttachments ) {
		const auto& attachment = it.second;
		bool textureHasMipMap = ( attachment->mTexture && attachment->mTexture->hasMipmapping() );
		bool resolveHasMipMap = ( attachment->mResolve && attachment->mResolve->hasMipmapping() );
		if( textureHasMipMap || resolveHasMipMap ) {
			mNeedsMipmapUpdate = true;
		}
	}
	*/

	/*
	for( const auto &textureAttachment : mAttachmentsTexture ) {
		if( textureAttachment.second->hasMipmapping() ) {
			mNeedsMipmapUpdate = true;
		}
	}
	*/
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

GLint Fbo::getNumSampleCounts( GLenum internalFormat )
{
	if( sNumSampleCounts.end() == sNumSampleCounts.find( internalFormat ) ) {
		GLint numSampleCounts = 0;
		glGetInternalformativ( GL_RENDERBUFFER, internalFormat, GL_NUM_SAMPLE_COUNTS, 1, &numSampleCounts );
		sNumSampleCounts[internalFormat] = numSampleCounts;
	}
	return sNumSampleCounts[internalFormat];
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

Surface8u Fbo::readPixels8u( const Area &area, GLenum attachment ) const
{
	return Surface8u();

/*
	// resolve first, before our own bind so that we don't force a resolve unnecessarily
	resolveTextures();
	ScopedFramebuffer readScp( GL_FRAMEBUFFER, mId );

	// we need to determine the bounds of the attachment so that we can crop against it and subtract from its height
	Area attachmentBounds = getBounds();
	auto attachedBufferIt = mAttachmentsBuffer.find( attachment );
	if( attachedBufferIt != mAttachmentsBuffer.end() ) {
		attachmentBounds = attachedBufferIt->second->getBounds();
	}
	else {
		auto attachedTextureIt = mAttachmentsTexture.find( attachment );	
		// a texture attachment can be either of type Texture2d or TextureCubeMap but this only makes sense for the former
		if( attachedTextureIt != mAttachmentsTexture.end() ) {
			if( typeid(*(attachedTextureIt->second)) == typeid(Texture2d) ) {
				attachmentBounds = static_cast<const Texture2d*>( attachedTextureIt->second.get() )->getBounds();
			}
			else {
				CI_LOG_W( "Reading from an unsupported texture attachment" );	
			}
		}
		else { // the user has attempted to read from an attachment we have no record of
			CI_LOG_W( "Reading from unknown attachment" );
		}
	}
	
	Area clippedArea = area.getClipBy( attachmentBounds );

#if ! defined( CINDER_GL_ES_2 )	
	glReadBuffer( attachment );
#endif
	Surface8u result( clippedArea.getWidth(), clippedArea.getHeight(), true );
	glReadPixels( clippedArea.x1, attachmentBounds.getHeight() - clippedArea.y2, clippedArea.getWidth(), clippedArea.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, result.getData() );
	
	// glReadPixels returns pixels which are bottom-up
	ip::flipVertical( &result );
	
	// by binding we marked ourselves as needing to be resolved, but since this was a read-only
	// operation and we resolved at the top, we can mark ourselves as not needing resolve
	mNeedsResolve = false;
	
	return result;
*/

/*
	// resolve first, before our own bind so that we don't force a resolve unnecessarily
	resolveTextures();
	ScopedFramebuffer readScp( GL_FRAMEBUFFER, mId );

	// we need to determine the bounds of the attachment so that we can crop against it and subtract from its height
	Area attachmentBounds = getBounds();
	auto attachedBufferIt = mAttachmentsBuffer.find( attachment );
	if( attachedBufferIt != mAttachmentsBuffer.end() )
		attachmentBounds = attachedBufferIt->second->getBounds();
	else {
		auto attachedTextureIt = mAttachmentsTexture.find( attachment );	
		// a texture attachment can be either of type Texture2d or TextureCubeMap but this only makes sense for the former
		if( attachedTextureIt != mAttachmentsTexture.end() ) {
			if( typeid(*(attachedTextureIt->second)) == typeid(Texture2d) )
				attachmentBounds = static_cast<const Texture2d*>( attachedTextureIt->second.get() )->getBounds();
			else
				CI_LOG_W( "Reading from an unsupported texture attachment" );	
		}
		else // the user has attempted to read from an attachment we have no record of
			CI_LOG_W( "Reading from unknown attachment" );
	}
	
	Area clippedArea = area.getClipBy( attachmentBounds );

#if ! defined( CINDER_GL_ES_2 )	
	glReadBuffer( attachment );
#endif
	Surface8u result( clippedArea.getWidth(), clippedArea.getHeight(), true );
	glReadPixels( clippedArea.x1, attachmentBounds.getHeight() - clippedArea.y2, clippedArea.getWidth(), clippedArea.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, result.getData() );
	
	// glReadPixels returns pixels which are bottom-up
	ip::flipVertical( &result );
	
	// by binding we marked ourselves as needing to be resolved, but since this was a read-only
	// operation and we resolved at the top, we can mark ourselves as not needing resolve
	mNeedsResolve = false;
	
	return result;
*/
}

bool Fbo::checkStatus()
{
	FboExceptionInvalidSpecification exc;
	bool status = checkStatus( &exc );
	if( ! status ) // failed creation; throw
		throw exc;
	return status;
}

bool Fbo::hasDepthAttachment() const 
{
#if defined( CINDER_GL_ES_2 )
	return ( mAttachments.find( GL_DEPTH_ATTACHMENT ) != mAttachments.end() );
#else
	return ( mAttachments.find( GL_DEPTH_ATTACHMENT ) != mAttachments.end() ) ||
		   ( mAttachments.find( GL_DEPTH_STENCIL_ATTACHMENT ) != mAttachments.end() );
#endif
}

/*
bool Fbo::hasDepthAttachment() const 
{ 
	return 
#if ! defined( CINDER_GL_ES_2 )
		mAttachmentsBuffer.find( GL_DEPTH_STENCIL_ATTACHMENT ) != mAttachmentsBuffer.end() 
		|| mAttachmentsTexture.find( GL_DEPTH_STENCIL_ATTACHMENT ) != mAttachmentsTexture.end() ||
#endif
		mAttachmentsBuffer.find( GL_DEPTH_ATTACHMENT ) != mAttachmentsBuffer.end() 
		|| mAttachmentsTexture.find( GL_DEPTH_ATTACHMENT ) != mAttachmentsTexture.end(); 
}
*/

bool Fbo::hasStencilAttachment() const 
{

#if defined( CINDER_GL_ES_2 )
	return ( mAttachments.find( GL_STENCIL_ATTACHMENT ) != mAttachments.end() );
#else
	return ( mAttachments.find( GL_STENCIL_ATTACHMENT ) != mAttachments.end() ) ||
		   ( mAttachments.find( GL_DEPTH_STENCIL_ATTACHMENT ) != mAttachments.end() );
#endif
}

/*
bool Fbo::hasStencilAttachment() const 
{ 
	return 
#if ! defined( CINDER_GL_ES_2 )
		mAttachmentsBuffer.find( GL_DEPTH_STENCIL_ATTACHMENT ) != mAttachmentsBuffer.end()
		|| mAttachmentsTexture.find( GL_DEPTH_STENCIL_ATTACHMENT ) != mAttachmentsTexture.end() ||
#endif
		mAttachmentsBuffer.find( GL_STENCIL_ATTACHMENT ) != mAttachmentsBuffer.end()  
		|| mAttachmentsTexture.find( GL_STENCIL_ATTACHMENT ) != mAttachmentsTexture.end();
}
*/

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
	os << "ID: " << rhs.mId;
	if( rhs.mMultisampleFramebufferId ) {
		os << "  Multisample ID: " << rhs.mMultisampleFramebufferId;
	}
	os << std::endl;
	if( ! rhs.mLabel.empty() ) {
		os << "  Label: " << rhs.mLabel << std::endl;
	}
	os << "   Dims: " << rhs.mWidth << " x " << rhs.mHeight << std::endl;
	for( const auto& it : rhs.mAttachments ) {
		GLenum attachmentPoint = it.first;
		const auto& attachment = it.second;
		if( attachment->mTexture ) {
			os << "-Texture Attachment: " << gl::constantToString( attachmentPoint ) << std::endl;
			os << *(attachment->mTexture) << std::endl;
		}
		else if( attachment->mBuffer ) {
			os << "-Renderbuffer Attachment: " << gl::constantToString( attachmentPoint ) << std::endl;
			os << *(attachment->mBuffer) << std::endl;
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
	//@TODO: FIXME: amendedFormat.attach( GL_COLOR_ATTACHMENT0, textureCubeMap );
	
	return FboCubeMapRef( new FboCubeMap( faceWidth, faceHeight, amendedFormat, textureCubeMap ) );
}

FboCubeMap::FboCubeMap( int32_t faceWidth, int32_t faceHeight, const Format &format, const TextureCubeMapRef &textureCubeMap )
	: Fbo( faceWidth, faceHeight, format ), mTextureCubeMap( textureCubeMap )
{
}

void FboCubeMap::bindFramebufferFace( GLenum faceTarget, GLint level, GLenum target, GLenum attachment )
{
	bindFramebuffer( target );
	framebufferTexture2d( target, attachment, faceTarget, mTextureCubeMap->getId(), level );
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
