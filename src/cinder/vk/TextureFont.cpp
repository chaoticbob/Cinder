/*
 Copyright 2016 Google Inc.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.


 Copyright (c) 2016, The Cinder Project, All rights reserved.

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

#include "cinder/vk/TextureFont.h"
#include "cinder/vk/CommandBuffer.h"
#include "cinder/vk/Context.h"
#include "cinder/vk/Descriptor.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/Pipeline.h"
#include "cinder/vk/PipelineSelector.h"
#include "cinder/vk/RenderPass.h"
#include "cinder/vk/ShaderProg.h"
#include "cinder/vk/UniformView.h"
#include "cinder/vk/scoped.h"

#include "cinder/Text.h"
#include "cinder/ip/Fill.h"
#include "cinder/ip/Premultiply.h"
	#include "cinder/ImageIo.h"
	#include "cinder/Rand.h"
	#include "cinder/Utilities.h"
#if defined( CINDER_MSW )
	#include <Windows.h>
#elif defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	#include "cinder/linux/FreeTypeUtil.h" 
#endif
#include "cinder/Unicode.h"

#include <set>

using std::unordered_map;

using namespace std;

namespace cinder { namespace vk {

#if defined( CINDER_MSW )
set<Font::Glyph> getNecessaryGlyphs( const Font &font, const string &supportedChars )
{
	set<Font::Glyph> result;

	GCP_RESULTS gcpResults;
	WCHAR *glyphIndices = NULL;

	std::u16string utf16 = toUtf16( supportedChars );

	::SelectObject( Font::getGlobalDc(), font.getHfont() );

	gcpResults.lStructSize = sizeof(gcpResults);
	gcpResults.lpOutString = NULL;
	gcpResults.lpOrder = NULL;
	gcpResults.lpCaretPos = NULL;
	gcpResults.lpClass = NULL;

	uint32_t bufferSize = std::max<uint32_t>( (uint32_t)(utf16.length() * 1.2f), 16);		// Initially guess number of chars plus a few
	while( true ) {
		if( glyphIndices ) {
			free( glyphIndices );
			glyphIndices = NULL;
		}

		glyphIndices = (WCHAR*)malloc( bufferSize * sizeof(WCHAR) );
		gcpResults.nGlyphs = bufferSize;
		gcpResults.lpDx = 0;
		gcpResults.lpGlyphs = glyphIndices;

		if( ! ::GetCharacterPlacementW( Font::getGlobalDc(), (wchar_t*)utf16.c_str(), utf16.length(), 0,
						&gcpResults, GCP_LIGATE | GCP_DIACRITIC | GCP_GLYPHSHAPE | GCP_REORDER ) ) {
			return set<Font::Glyph>(); // failure
		}

		if( gcpResults.lpGlyphs )
			break;

		// Too small a buffer, try again
		bufferSize += bufferSize / 2;
		if( bufferSize > INT_MAX) {
			return set<Font::Glyph>(); // failure
		}
	}

	for( UINT i = 0; i < gcpResults.nGlyphs; i++ )
		result.insert( glyphIndices[i] );

	if( glyphIndices )
		free( glyphIndices );

	return result;
}

TextureFont::TextureFont( const Font &font, const string &utf8Chars, const Format &format )
	: mFont( font ), mFormat( format )
{
	// get the glyph indices we'll need
	set<Font::Glyph> glyphs = getNecessaryGlyphs( font, utf8Chars );
	// determine the max glyph extents
	ivec2 glyphExtents;
	for( set<Font::Glyph>::const_iterator glyphIt = glyphs.begin(); glyphIt != glyphs.end(); ++glyphIt ) {
		try {
			Rectf bb = font.getGlyphBoundingBox( *glyphIt );
			glyphExtents.x = std::max<int>( glyphExtents.x, bb.getWidth() );
			glyphExtents.y = std::max<int>( glyphExtents.y, bb.getHeight() );
		}
		catch( FontGlyphFailureExc &e ) {
		}
	}

	::SelectObject( Font::getGlobalDc(), mFont.getHfont() );

	if( ( glyphExtents.x == 0 ) || ( glyphExtents.y == 0 ) )
		return;

	int glyphsWide = mFormat.getTextureWidth() / glyphExtents.x;
	int glyphsTall = mFormat.getTextureHeight() / glyphExtents.y;	
	uint8_t curGlyphIndex = 0, curTextureIndex = 0;
	ivec2 curOffset = ivec2( 0, 0 );

	Channel channel( mFormat.getTextureWidth(), mFormat.getTextureHeight() );
	ip::fill<uint8_t>( &channel, 0 );
	std::unique_ptr<uint8_t[]> lumAlphaData( new uint8_t[mFormat.getTextureWidth()*mFormat.getTextureHeight()*2] );

	GLYPHMETRICS gm = { 0, };
	MAT2 identityMatrix = { {0,1},{0,0},{0,0},{0,1} };
	size_t bufferSize = 1;
	BYTE *pBuff = new BYTE[bufferSize];
	for( set<Font::Glyph>::const_iterator glyphIt = glyphs.begin(); glyphIt != glyphs.end(); ) {
		DWORD dwBuffSize = ::GetGlyphOutline( Font::getGlobalDc(), *glyphIt, GGO_GRAY8_BITMAP | GGO_GLYPH_INDEX, &gm, 0, NULL, &identityMatrix );
		if( dwBuffSize == GDI_ERROR ) {
			++glyphIt;
			continue;
		}
		if( dwBuffSize > bufferSize ) {
			delete[] pBuff;
			pBuff = new BYTE[dwBuffSize];
			bufferSize = dwBuffSize;
		}
		else if( dwBuffSize == 0 ) {
			++glyphIt;
			continue;
		}

		if( ::GetGlyphOutline( Font::getGlobalDc(), *glyphIt, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL, &identityMatrix ) == GDI_ERROR ) {
			++glyphIt;
			continue;
		}

		if( ::GetGlyphOutline( Font::getGlobalDc(), *glyphIt, GGO_GRAY8_BITMAP | GGO_GLYPH_INDEX, &gm, dwBuffSize, pBuff, &identityMatrix ) == GDI_ERROR ) {
			++glyphIt;
			continue;
		}

		// convert 6bit to 8bit gray
		for( INT p = 0; p < dwBuffSize; ++p )
			pBuff[p] = ((uint32_t)pBuff[p]) * 255 / 64;

		int32_t alignedRowBytes = ( gm.gmBlackBoxX & 3 ) ? ( gm.gmBlackBoxX + 4 - ( gm.gmBlackBoxX & 3 ) ) : gm.gmBlackBoxX;
		Channel glyphChannel( gm.gmBlackBoxX, gm.gmBlackBoxY, alignedRowBytes, 1, pBuff );
		channel.copyFrom( glyphChannel, glyphChannel.getBounds(), curOffset );

		GlyphInfo newInfo;
		newInfo.mOriginOffset = vec2( gm.gmptGlyphOrigin.x, glyphExtents.y - gm.gmptGlyphOrigin.y );
		newInfo.mTexCoords = Area( curOffset, curOffset + ivec2( gm.gmBlackBoxX, gm.gmBlackBoxY ) );
		newInfo.mTextureIndex = curTextureIndex;
		mGlyphMap[*glyphIt] = newInfo;

		curOffset += ivec2( glyphExtents.x, 0 );
		++glyphIt;
		if( ( ++curGlyphIndex == glyphsWide * glyphsTall ) || ( glyphIt == glyphs.end() ) ) {
			ci::Surface tempSurface( channel, SurfaceConstraintsDefault(), true );
			tempSurface.getChannelAlpha().copyFrom( channel, channel.getBounds() );
			if( ! format.getPremultiply() )
				ip::unpremultiply( &tempSurface );
			
			vk::Texture::Format textureFormat = vk::Texture::Format();
			textureFormat.mipmap( mFormat.hasMipmapping() );

			writeImage( "TextureFont.png", tempSurface );

			Surface8u::ConstIter iter( tempSurface, tempSurface.getBounds() );
			size_t offset = 0;
			while( iter.line() ) {
				while( iter.pixel() ) {
					lumAlphaData.get()[offset+0] = iter.r();
					lumAlphaData.get()[offset+1] = iter.a();
					offset += 2;
				}
			}

			textureFormat.setInternalFormat( VK_FORMAT_R8G8_UNORM );
			VkComponentMapping swizzle = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G };
			textureFormat.swizzle( swizzle );

			auto texture = vk::Texture::create( lumAlphaData.get(), textureFormat.getInternalFormat(), mFormat.getTextureWidth(), mFormat.getTextureHeight(), textureFormat );
			mTextures.push_back( texture );
			ip::fill<uint8_t>( &channel, 0 );			
			curOffset = ivec2( 0, 0 );
			curGlyphIndex = 0;
			++curTextureIndex;
		}
		else if( ( curGlyphIndex ) % glyphsWide == 0 ) { // wrap around
			curOffset.x = 0;
			curOffset.y += glyphExtents.y;
		}
	}

	delete [] pBuff;
}

#elif defined( CINDER_ANDROID ) || defined( CINDER_LINUX )

TextureFont::TextureFont( const Font &font, const string &utf8Chars, const Format &format )
	: mFont( font ), mFormat( format )
{
	FT_Face face = font.getFreetypeFace();
	std::u32string utf32Chars = ci::toUtf32( utf8Chars );
	// Add a space if needed
	if( std::string::npos == utf8Chars.find( ' ' ) ) {
		utf32Chars += ci::toUtf32( " " );
	}

	// get the glyph indices we'll need
	set<Font::Glyph> glyphs;
	for( const auto& ch : utf32Chars ) {
		FT_UInt glyphIndex = FT_Get_Char_Index( face, ch );
		glyphs.insert( glyphIndex );
	}

	// determine the max glyph extents
	vec2 glyphExtents;
	for( set<Font::Glyph>::const_iterator glyphIt = glyphs.begin(); glyphIt != glyphs.end(); ++glyphIt ) {
		Rectf bb = font.getGlyphBoundingBox( *glyphIt );
		glyphExtents.x = std::max( glyphExtents.x, bb.getWidth() );
		glyphExtents.y = std::max( glyphExtents.y, bb.getHeight() );
	}

	glyphExtents.x = ceil( glyphExtents.x );
	glyphExtents.y = ceil( glyphExtents.y );

	int glyphsWide = floor( mFormat.getTextureWidth() / (glyphExtents.x+3) );
	int glyphsTall = floor( mFormat.getTextureHeight() / (glyphExtents.y+5) );	
	uint8_t curGlyphIndex = 0, curTextureIndex = 0;
	vec2 curOffset;
	std::vector<FT_UInt> renderGlyphs( glyphsWide*glyphsTall );
	std::vector<FT_Vector> renderPositions( glyphsWide*glyphsTall );
	Surface surface( mFormat.getTextureWidth(), mFormat.getTextureHeight(), true );
	ip::fill( &surface, ColorA8u( 0, 0, 0, 0 ) );
	ColorA white( 1, 1, 1, 1 );
	ivec2 surfaceSize		= surface.getSize();
	uint8_t* surfaceData   	= surface.getData();
	size_t surfacePixelInc 	= surface.getPixelInc();
	size_t surfaceRowBytes 	= surface.getRowBytes();	

	std::unique_ptr<uint8_t[]> lumAlphaData( new uint8_t[mFormat.getTextureWidth()*mFormat.getTextureHeight()*2] );

	for( set<Font::Glyph>::const_iterator glyphIt = glyphs.begin(); glyphIt != glyphs.end(); ) {
		GlyphInfo newInfo;
		newInfo.mTextureIndex = curTextureIndex;
		Rectf bb = font.getGlyphBoundingBox( *glyphIt );
		vec2 ul = curOffset + vec2( 0, glyphExtents.y - bb.getHeight() );
		vec2 lr = curOffset + vec2( glyphExtents.x, glyphExtents.y );
		newInfo.mTexCoords = Area( floor( ul.x ), floor( ul.y ), ceil( lr.x ) + 3, ceil( lr.y ) + 2 );
		newInfo.mOriginOffset.x = floor(bb.x1) - 1;
		newInfo.mOriginOffset.y = -(bb.getHeight()-1)-ceil( bb.y1+0.5f );
		mGlyphMap[*glyphIt] = newInfo;
		renderGlyphs[curGlyphIndex] = *glyphIt;
		renderPositions[curGlyphIndex].x = curOffset.x - floor(bb.x1) + 1;
		renderPositions[curGlyphIndex].y = surface.getHeight() - (curOffset.y + glyphExtents.y) - ceil(bb.y1+0.5f);
		curOffset += ivec2( glyphExtents.x + 3, 0 );
		++glyphIt;
		if( ( ++curGlyphIndex == glyphsWide * glyphsTall ) || ( glyphIt == glyphs.end() ) ) {
			for( size_t i = 0; i < (size_t)curGlyphIndex; ++i ) {
				FT_UInt glyphIndex = renderGlyphs[i];
				FT_Vector glyphPos = renderPositions[i];
				FT_Vector pen = { glyphPos.x*64, glyphPos.y*64 };
				FT_Set_Transform( face, nullptr, &pen );
				FT_Load_Glyph( face, glyphIndex, FT_LOAD_RENDER );
				FT_GlyphSlot slot = face->glyph;
				ivec2 drawOffset = ivec2( slot->bitmap_left, surfaceSize.y - slot->bitmap_top );
				ci::linux::ftutil::DrawBitmap( drawOffset, &(slot->bitmap), white, surfaceData, surfacePixelInc, surfaceRowBytes, surfaceSize );

				
				FT_Load_Glyph( face, glyphIndex, FT_LOAD_DEFAULT );
				Font::GlyphMetrics glyphMetrics;
				glyphMetrics.advance = ivec2( slot->advance.x, slot->advance.y );
				//glyphMetrics.metrics = slot->metrics;
				mCachedGlyphMetrics[glyphIndex] = glyphMetrics;
			}

			// pass premultiply and mipmapping preferences to Texture::Format
			if( ! mFormat.getPremultiply() ) {
				ip::unpremultiply( &surface );
			}

			gl::Texture::Format textureFormat = gl::Texture::Format();
			textureFormat.mipmap( mFormat.hasMipmapping() );
			GLint dataFormat;
#if defined( CINDER_GL_ES )
			dataFormat = GL_LUMINANCE_ALPHA;
			textureFormat.setInternalFormat( dataFormat );
#else
			dataFormat = GL_RG;
			textureFormat.setInternalFormat( dataFormat );
			textureFormat.setSwizzleMask( { GL_RED, GL_RED, GL_RED, GL_GREEN } );
#endif
			if( mFormat.hasMipmapping() )
				textureFormat.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );

			// under iOS format and interalFormat must match, so let's make a block of LUMINANCE_ALPHA data
			Surface8u::ConstIter iter( surface, surface.getBounds() );
			size_t offset = 0;
			while( iter.line() ) {
				while( iter.pixel() ) {
					lumAlphaData.get()[offset+0] = iter.r();
					lumAlphaData.get()[offset+1] = iter.a();
					offset += 2;
				}
			}
			mTextures.push_back( gl::Texture::create( lumAlphaData.get(), dataFormat, mFormat.getTextureWidth(), mFormat.getTextureHeight(), textureFormat ) );
			mTextures.back()->setTopDown( true );

			ip::fill( &surface, ColorA8u( 0, 0, 0, 0 ) );			
			curOffset = vec2();
			curGlyphIndex = 0;
			++curTextureIndex;
		}
		else if( ( curGlyphIndex ) % glyphsWide == 0 ) { // wrap around
			curOffset.x = 0;
			curOffset.y += glyphExtents.y + 2;
		}
	}
}

#endif

static void drawGlyphsBuffers( 
	const vk::ShaderProgRef&		shader, 
	const vk::UniformViewRef&		transientUniformSet, 
	const vk::DescriptorSetViewRef&	transientDescriptorSetView,
	const vk::TextureRef&			curTex,
	const vk::IndexBufferRef&		transientIndexBuffer, 
	const vk::VertexBufferRef&		transientVertexBufferVerts, 
	const vk::VertexBufferRef&		transientVertexBufferTexCoords, 
	const vk::VertexBufferRef&		transientVertexBufferVertColors 
)
{
	// Descriptor set layouts
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = vk::context()->getDevice()->getDescriptorSetLayoutSelector()->getSelectedLayout( transientUniformSet->getDescriptorSetLayoutBindings() );

	// Pipeline layout
	VkPipelineLayout pipelineLayout = vk::context()->getDevice()->getPipelineLayoutSelector()->getSelectedLayout( descriptorSetLayouts );

	// Pipeline
	VkPipeline pipeline = VK_NULL_HANDLE;
	{
		const VkFormat formatVert		= VK_FORMAT_R32G32_SFLOAT;
		const VkFormat formatTexCoord	= VK_FORMAT_R32G32_SFLOAT;
		const VkFormat formatVertColor	= VK_FORMAT_R8G8B8A8_UNORM;

		// Vertex input binding description
		// Position
		VkVertexInputBindingDescription viBindingPos = {};
		viBindingPos.binding			= 0;
		viBindingPos.inputRate			= VK_VERTEX_INPUT_RATE_VERTEX;
		viBindingPos.stride				= vk::formatSizeBytes( formatVert );
		// TexCoord
		VkVertexInputBindingDescription viBindingTexCoord = {};
		viBindingTexCoord.binding		= 1;
		viBindingTexCoord.inputRate		= VK_VERTEX_INPUT_RATE_VERTEX;
		viBindingTexCoord.stride		= vk::formatSizeBytes( formatTexCoord );
		// Color
		VkVertexInputBindingDescription viBindingVertColor = {};
		viBindingVertColor.binding		= 2;
		viBindingVertColor.inputRate	= VK_VERTEX_INPUT_RATE_VERTEX;
		viBindingVertColor.stride		= vk::formatSizeBytes( formatVertColor );

		// Vertex input attribute description
		// Position
		VkVertexInputAttributeDescription viAttrPos = {};
		viAttrPos.binding				= viBindingPos.binding;
		viAttrPos.format				= formatVert;
		viAttrPos.location				= shader->getAttributeLocation( geom::Attrib::POSITION );
		viAttrPos.offset				= 0;
		// TexCoord
		VkVertexInputAttributeDescription viAttrTexCoord = {};
		viAttrTexCoord.binding			= viBindingTexCoord.binding;
		viAttrTexCoord.format			= formatTexCoord;
		viAttrTexCoord.location			= shader->getAttributeLocation( geom::Attrib::TEX_COORD_0 );
		viAttrTexCoord.offset			= 0;
		// Color
		VkVertexInputAttributeDescription viAttrVertColor = {};
		viAttrVertColor.binding			= viBindingVertColor.binding;
		viAttrVertColor.format			= formatVertColor;
		viAttrVertColor.location		= shader->getAttributeLocation( geom::Attrib::COLOR );
		viAttrVertColor.offset			= 0;


		auto ctx = vk::context();
		auto& pipelineSelector = ctx->getDevice()->getPipelineSelector();
		pipelineSelector->setTopology( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST );
		pipelineSelector->setVertexInputAttributeDescriptions( { viAttrPos, viAttrTexCoord, viAttrVertColor } );
		pipelineSelector->setVertexInputBindingDescriptions( { viBindingPos, viBindingTexCoord, viBindingVertColor } );
		pipelineSelector->setCullMode( ctx->getCullMode() );
		pipelineSelector->setFrontFace( ctx->getFrontFace() );
		pipelineSelector->setDepthBias( ctx->getDepthBiasEnable(), ctx->getDepthBiasSlopeFactor(), ctx->getDepthBiasConstantFactor(), ctx->getDepthBiasClamp() );
		pipelineSelector->setRasterizationSamples( ctx->getRenderPass()->getSubpassSampleCount( ctx->getSubpass() ) );
		pipelineSelector->setDepthTest( ctx->getDepthTest() );
		pipelineSelector->setDepthWrite( ctx->getDepthWrite() );
		pipelineSelector->setColorBlendAttachments( ctx->getColorBlendAttachments() );
		pipelineSelector->setShaderStages( shader->getShaderStages() );
		pipelineSelector->setRenderPass( ctx->getRenderPass()->vk() );
		pipelineSelector->setSubPass( ctx->getSubpass() );
		pipelineSelector->setPipelineLayout( pipelineLayout );
		pipeline = pipelineSelector->getSelectedPipeline();
	}

	// -------------------------------------------------------------------------------------------------------------------------------------------

	// Command buffer
	auto cmdBufRef = vk::context()->getCommandBuffer();
	auto cmdBuf = cmdBufRef->vk();

	// Fill out uniform vars
	transientUniformSet->uniform( "uTex0", curTex );
	transientUniformSet->setDefaultUniformVars( vk::context() );
	transientUniformSet->bufferPending( cmdBufRef );
	cmdBufRef->pipelineBarrierGlobalMemoryUniformTransfer();

	// Update descriptor set
	transientDescriptorSetView->updateDescriptorSets();

	// Bind index buffer
	cmdBufRef->bindIndexBuffer( transientIndexBuffer );

	// Bind vertex buffer
	cmdBufRef->bindVertexBuffers( { transientVertexBufferVerts, transientVertexBufferTexCoords, transientVertexBufferVertColors } );

	// Bind pipeline
	cmdBufRef->bindPipeline( VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );

	// Bind descriptor sets
	const auto& descriptorSets = transientDescriptorSetView->getDescriptorSets();
	for( uint32_t i = 0; i < descriptorSets.size(); ++i ) {
		const auto& ds = descriptorSets[i];
		std::vector<VkDescriptorSet> descSets = { ds->vk() };
		cmdBufRef->bindDescriptorSets( VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, i, static_cast<uint32_t>( descSets.size() ), descSets.data(), 0, nullptr );
	}

	// Draw geometry
	uint32_t indexCount = transientIndexBuffer->getNumIndices();
	cmdBufRef->drawIndexed( indexCount, 1, 0, 0, 0 );
}

void TextureFont::drawGlyphs( const vector<pair<Font::Glyph,vec2>> &glyphMeasures, const vec2 &baselineIn, const DrawOptions &options, const std::vector<ColorA8u> &colors )
{
	if( mTextures.empty() ) {
		return;
	}

	if( ! colors.empty() ) {
		assert( glyphMeasures.size() == colors.size() );
	}

	auto shader = options.getShaderProg();
	if( ! shader ) {
		auto shaderDef = ShaderDef().texture().color();
		shader = vk::getStockShader( shaderDef );
	}

	auto currentColor = vk::context()->getCurrentColor();

	vec2 baseline = baselineIn;
	const float scale = options.getScale();
	for( size_t texIdx = 0; texIdx < mTextures.size(); ++texIdx ) {
		std::vector<vec2> verts, texCoords;
		std::vector<ColorA8u> vertColors;
		const vk::TextureRef &curTex = mTextures[texIdx];
		std::vector<uint32_t> indices;
		uint32_t curIdx = 0;
		VkIndexType indexType = VK_INDEX_TYPE_UINT32;

		if( options.getPixelSnap() ) {
			baseline = vec2( floor( baseline.x ), floor( baseline.y ) );
		}
			
		for( vector<pair<Font::Glyph,vec2> >::const_iterator glyphIt = glyphMeasures.begin(); glyphIt != glyphMeasures.end(); ++glyphIt ) {
			unordered_map<Font::Glyph, GlyphInfo>::const_iterator glyphInfoIt = mGlyphMap.find( glyphIt->first );
			if( (glyphInfoIt == mGlyphMap.end()) || (mGlyphMap[glyphIt->first].mTextureIndex != texIdx) ) {
				continue;
			}
				
			const GlyphInfo &glyphInfo = glyphInfoIt->second;
			
			Rectf destRect( glyphInfo.mTexCoords );
			Rectf srcTexCoords = curTex->getAreaTexCoords( glyphInfo.mTexCoords );
			destRect -= destRect.getUpperLeft();
			destRect.scale( scale );
			destRect += glyphIt->second * scale;
			destRect += vec2( floor( glyphInfo.mOriginOffset.x + 0.5f ), floor( glyphInfo.mOriginOffset.y ) ) * scale;
			destRect += vec2( baseline.x, baseline.y - mFont.getAscent() * scale );
			if( options.getPixelSnap() ) {
				destRect -= vec2( destRect.x1 - floor( destRect.x1 ), destRect.y1 - floor( destRect.y1 ) );				
			}
			
			verts.push_back( vec2( destRect.getX2(), destRect.getY1() ) );
			verts.push_back( vec2( destRect.getX1(), destRect.getY1() ) );
			verts.push_back( vec2( destRect.getX2(), destRect.getY2() ) );
			verts.push_back( vec2( destRect.getX1(), destRect.getY2() ) );

			texCoords.push_back( vec2( srcTexCoords.getX2(), srcTexCoords.getY1() ) );
			texCoords.push_back( vec2( srcTexCoords.getX1(), srcTexCoords.getY1() ) );
			texCoords.push_back( vec2( srcTexCoords.getX2(), srcTexCoords.getY2() ) );
			texCoords.push_back( vec2( srcTexCoords.getX1(), srcTexCoords.getY2() ) );
			
			if( ! colors.empty() ) {
				for( int i = 0; i < 4; ++i ) {
					vertColors.push_back( colors[glyphIt-glyphMeasures.begin()] );
				}
			}
			else {
				for( int i = 0; i < 4; ++i ) {
					vertColors.push_back( currentColor );
				}
			}

			indices.push_back( curIdx + 0 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 2 );
			indices.push_back( curIdx + 2 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 3 );
			curIdx += 4;
		}
		
		if( curIdx == 0 ) {
			continue;
		}

		// -------------------------------------------------------------------------------------------------------------------------------------------
		
		// Index buffer
		vk::IndexBuffer::Format indexBufferFormat = vk::IndexBuffer::Format( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ).setUsageTransferDestination();
		vk::IndexBufferRef transientIndexBuffer = vk::IndexBuffer::create( indices.size(), indexType, static_cast<const void *>( indices.data() ), indexBufferFormat );
		vk::context()->addTransient( transientIndexBuffer );
		
		// Vertex buffers
		vk::VertexBuffer::Format vertexBufferFormat = vk::VertexBuffer::Format( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ).setUsageTransferDestination();
		vk::VertexBufferRef transientVertexBufferVerts = vk::VertexBuffer::create( static_cast<const void *>( verts.data() ), verts.size()*sizeof( vec2 ), vertexBufferFormat );
		vk::VertexBufferRef transientVertexBufferTexCoords = vk::VertexBuffer::create( static_cast<const void *>( texCoords.data() ), texCoords.size()*sizeof( vec2 ), vertexBufferFormat );
		vk::VertexBufferRef transientVertexBufferVertColors = vk::VertexBuffer::create( static_cast<const void *>( vertColors.data() ), vertColors.size()*sizeof( ColorA8u ), vertexBufferFormat );
		vk::context()->addTransient( transientVertexBufferVerts );
		vk::context()->addTransient( transientVertexBufferTexCoords );
		vk::context()->addTransient( transientVertexBufferVertColors );

		// Uniform layout, uniform set
		const vk::UniformLayout& uniformLayout = shader->getUniformLayout();
		vk::UniformView::Options uniformSetOptions = vk::UniformView::Options().setTransientAllocation();
		vk::UniformViewRef transientUniformSet = vk::UniformView::create( uniformLayout, uniformSetOptions );
		vk::context()->addTransient( transientUniformSet );
		
		// Descriptor set view
		vk::DescriptorSetViewRef transientDescriptorSetView = vk::DescriptorSetView::create( transientUniformSet );
		transientDescriptorSetView->allocateDescriptorSets();
		vk::context()->addTransient( transientDescriptorSetView );

		drawGlyphsBuffers( shader, transientUniformSet, transientDescriptorSetView, curTex, transientIndexBuffer, transientVertexBufferVerts, transientVertexBufferTexCoords, transientVertexBufferVertColors );
	}
}

void TextureFont::drawGlyphs( const std::vector<std::pair<Font::Glyph,vec2>> &glyphMeasures, const Rectf &clip, vec2 offset, const DrawOptions &options, const std::vector<ColorA8u> &colors )
{
	if( mTextures.empty() )
		return;

	if( ! colors.empty() ) {
		assert( glyphMeasures.size() == colors.size() );
	}

	auto shader = options.getShaderProg();
	if( ! shader ) {
		auto shaderDef = ShaderDef().texture().color();
		shader = vk::getStockShader( shaderDef );
	}

	auto currentColor = vk::context()->getCurrentColor();

	const float scale = options.getScale();

	for( size_t texIdx = 0; texIdx < mTextures.size(); ++texIdx ) {
		std::vector<vec2> verts, texCoords;
		std::vector<ColorA8u> vertColors;
		const vk::TextureRef &curTex = mTextures[texIdx];
		std::vector<uint32_t> indices;
		uint32_t curIdx = 0;
		VkIndexType indexType = VK_INDEX_TYPE_UINT32;

		if( options.getPixelSnap() ) {
			offset = vec2( floor( offset.x ), floor( offset.y ) );
		}

		for( vector<pair<Font::Glyph,vec2> >::const_iterator glyphIt = glyphMeasures.begin(); glyphIt != glyphMeasures.end(); ++glyphIt ) {
			unordered_map<Font::Glyph, GlyphInfo>::const_iterator glyphInfoIt = mGlyphMap.find( glyphIt->first );
			if( (glyphInfoIt == mGlyphMap.end()) || (mGlyphMap[glyphIt->first].mTextureIndex != texIdx) ) {
				continue;
			}
				
			const GlyphInfo &glyphInfo = glyphInfoIt->second;
			Rectf srcTexCoords = curTex->getAreaTexCoords( glyphInfo.mTexCoords );
			Rectf destRect( glyphInfo.mTexCoords );
			destRect -= destRect.getUpperLeft();
			destRect.scale( scale );
			destRect += glyphIt->second * scale;
			destRect += vec2( floor( glyphInfo.mOriginOffset.x + 0.5f ), floor( glyphInfo.mOriginOffset.y ) ) * scale;
			destRect += vec2( offset.x, offset.y );
			if( options.getPixelSnap() ) {
				destRect -= vec2( destRect.x1 - floor( destRect.x1 ), destRect.y1 - floor( destRect.y1 ) );	
			}

			// clip
			Rectf clipped( destRect );
			if( options.getClipHorizontal() ) {
				clipped.x1 = std::max( destRect.x1, clip.x1 );
				clipped.x2 = std::min( destRect.x2, clip.x2 );
			}
			if( options.getClipVertical() ) {
				clipped.y1 = std::max( destRect.y1, clip.y1 );
				clipped.y2 = std::min( destRect.y2, clip.y2 );
			}
			
			if( clipped.x1 >= clipped.x2 || clipped.y1 >= clipped.y2 )
				continue;
			
			vec2 coordScale( 1 / (float)destRect.getWidth() / curTex->getWidth() * glyphInfo.mTexCoords.getWidth(),
				1 / (float)destRect.getHeight() / curTex->getHeight() * glyphInfo.mTexCoords.getHeight() );
			srcTexCoords.x1 = srcTexCoords.x1 + ( clipped.x1 - destRect.x1 ) * coordScale.x;
			srcTexCoords.x2 = srcTexCoords.x1 + ( clipped.x2 - clipped.x1 ) * coordScale.x;
			srcTexCoords.y1 = srcTexCoords.y1 + ( clipped.y1 - destRect.y1 ) * coordScale.y;
			srcTexCoords.y2 = srcTexCoords.y1 + ( clipped.y2 - clipped.y1 ) * coordScale.y;

			verts.push_back( vec2( clipped.getX2(), clipped.getY1() ) );
			verts.push_back( vec2( clipped.getX1(), clipped.getY1() ) );
			verts.push_back( vec2( clipped.getX2(), clipped.getY2() ) );
			verts.push_back( vec2( clipped.getX1(), clipped.getY2() ) );

			texCoords.push_back( vec2( srcTexCoords.getX2(), srcTexCoords.getY1() ) );
			texCoords.push_back( vec2( srcTexCoords.getX1(), srcTexCoords.getY1() ) );
			texCoords.push_back( vec2( srcTexCoords.getX2(), srcTexCoords.getY2() ) );
			texCoords.push_back( vec2( srcTexCoords.getX1(), srcTexCoords.getY2() ) );
			
			if( ! colors.empty() ) {
				for( int i = 0; i < 4; ++i ) {
					vertColors.push_back( colors[glyphIt-glyphMeasures.begin()] );
				}
			}
			else {
				for( int i = 0; i < 4; ++i ) {
					vertColors.push_back( currentColor );
				}
			}
			
			indices.push_back( curIdx + 0 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 2 );
			indices.push_back( curIdx + 2 ); indices.push_back( curIdx + 1 ); indices.push_back( curIdx + 3 );
			curIdx += 4;
		}
		
		if( curIdx == 0 )
			continue;
		
		// -------------------------------------------------------------------------------------------------------------------------------------------
		
		// Index buffer
		vk::IndexBuffer::Format indexBufferFormat = vk::IndexBuffer::Format( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ).setUsageTransferDestination();
		vk::IndexBufferRef transientIndexBuffer = vk::IndexBuffer::create( indices.size(), indexType, static_cast<const void *>( indices.data() ), indexBufferFormat );
		vk::context()->addTransient( transientIndexBuffer );
		
		// Vertex buffers
		vk::VertexBuffer::Format vertexBufferFormat = vk::VertexBuffer::Format( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ).setUsageTransferDestination();
		vk::VertexBufferRef transientVertexBufferVerts = vk::VertexBuffer::create( static_cast<const void *>( verts.data() ), verts.size()*sizeof( vec2 ), vertexBufferFormat );
		vk::VertexBufferRef transientVertexBufferTexCoords = vk::VertexBuffer::create( static_cast<const void *>( texCoords.data() ), texCoords.size()*sizeof( vec2 ), vertexBufferFormat );
		vk::VertexBufferRef transientVertexBufferVertColors = vk::VertexBuffer::create( static_cast<const void *>( vertColors.data() ), vertColors.size()*sizeof( ColorA8u ), vertexBufferFormat );
		vk::context()->addTransient( transientVertexBufferVerts );
		vk::context()->addTransient( transientVertexBufferTexCoords );
		vk::context()->addTransient( transientVertexBufferVertColors );

		// Uniform layout, uniform set
		const vk::UniformLayout& uniformLayout = shader->getUniformLayout();
		vk::UniformView::Options uniformSetOptions = vk::UniformView::Options().setTransientAllocation();
		vk::UniformViewRef transientUniformSet = vk::UniformView::create( uniformLayout, uniformSetOptions );
		vk::context()->addTransient( transientUniformSet );
		
		// Descriptor view		
		vk::DescriptorSetViewRef transientDescriptorSetView = vk::DescriptorSetView::create( transientUniformSet );
		transientDescriptorSetView->allocateDescriptorSets();
		vk::context()->addTransient( transientDescriptorSetView );

		drawGlyphsBuffers( shader, transientUniformSet, transientDescriptorSetView, curTex, transientIndexBuffer, transientVertexBufferVerts, transientVertexBufferTexCoords, transientVertexBufferVertColors );
	}
}

void TextureFont::drawString( const std::string &str, const vec2 &baseline, const DrawOptions &options )
{
	TextBox tbox = TextBox().font( mFont ).text( str ).size( TextBox::GROW, TextBox::GROW ).ligate( options.getLigate() );
#if defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs( getCachedGlyphMetrics() );
#else
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs();
#endif	
	drawGlyphs( glyphMeasures, baseline, options );
}

void TextureFont::drawString( const std::string &str, const Rectf &fitRect, const vec2 &offset, const DrawOptions &options )
{
	TextBox tbox = TextBox().font( mFont ).text( str ).size( TextBox::GROW, fitRect.getHeight() ).ligate( options.getLigate() );
#if defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs( getCachedGlyphMetrics() );
#else
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs();
#endif	
	drawGlyphs( glyphMeasures, fitRect, fitRect.getUpperLeft() + offset, options );	
}

void TextureFont::drawStringWrapped( const std::string &str, const Rectf &fitRect, const vec2 &offset, const DrawOptions &options )
{
	TextBox tbox = TextBox().font( mFont ).text( str ).size( fitRect.getWidth(), fitRect.getHeight() ).ligate( options.getLigate() );
#if defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs( getCachedGlyphMetrics() );
#else
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs();
#endif	
	drawGlyphs( glyphMeasures, fitRect.getUpperLeft() + offset, options );
}

vec2 TextureFont::measureString( const std::string &str, const DrawOptions &options ) const
{
	TextBox tbox = TextBox().font( mFont ).text( str ).size( TextBox::GROW, TextBox::GROW ).ligate( options.getLigate() );

#if defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs( getCachedGlyphMetrics() );
#else
	vector<pair<Font::Glyph,vec2> > glyphMeasures = tbox.measureGlyphs();
#endif	
	if( ! glyphMeasures.empty() ) {
		vec2 result = glyphMeasures.back().second;
		unordered_map<Font::Glyph, GlyphInfo>::const_iterator glyphInfoIt = mGlyphMap.find( glyphMeasures.back().first );
		if( glyphInfoIt != mGlyphMap.end() )
			result += glyphInfoIt->second.mOriginOffset + vec2( glyphInfoIt->second.mTexCoords.getSize() );
		return result;
	}
	else {
		return vec2();
	}
}

vector<pair<Font::Glyph,vec2> > TextureFont::getGlyphPlacements( const std::string &str, const DrawOptions &options ) const
{
	TextBox tbox = TextBox().font( mFont ).text( str ).size( TextBox::GROW, TextBox::GROW ).ligate( options.getLigate() );
	return tbox.measureGlyphs();
}

vector<pair<Font::Glyph,vec2> > TextureFont::getGlyphPlacements( const std::string &str, const Rectf &fitRect, const DrawOptions &options ) const
{
	TextBox tbox = TextBox().font( mFont ).text( str ).size( TextBox::GROW, fitRect.getHeight() ).ligate( options.getLigate() );
	return tbox.measureGlyphs();
}

vector<pair<Font::Glyph,vec2> > TextureFont::getGlyphPlacementsWrapped( const std::string &str, const Rectf &fitRect, const DrawOptions &options ) const
{
	TextBox tbox = TextBox().font( mFont ).text( str ).size( fitRect.getWidth(), fitRect.getHeight() ).ligate( options.getLigate() );
	return tbox.measureGlyphs();
}

} } // namespace cinder::vk