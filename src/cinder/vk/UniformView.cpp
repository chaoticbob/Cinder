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

#include "cinder/vk/UniformView.h"
#include "cinder/vk/CommandBuffer.h"
#include "cinder/vk/Context.h"
#include "cinder/vk/Texture.h"
#include "cinder/vk/UniformBuffer.h"
#include "cinder/Utilities.h"

namespace cinder { namespace vk {

// -------------------------------------------------------------------------------------------------
// UniformSet::Binding
// -------------------------------------------------------------------------------------------------
void UniformView::Binding::setUniformBuffer( const UniformBufferRef& buffer )
{
	mUniformBuffer = buffer;
	setDirty();
}

// -------------------------------------------------------------------------------------------------
// UniformSet::Set
// -------------------------------------------------------------------------------------------------
std::vector<VkWriteDescriptorSet> UniformView::Set::getBindingUpdates( VkDescriptorSet parentDescriptorSet )
{
	std::vector<VkWriteDescriptorSet> result;

	for( auto& binding : mBindings ) {
		if( ! binding.isDirty() ) {
			continue;
		}

		VkWriteDescriptorSet entry = {};
		entry.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		entry.pNext				= NULL;
		entry.dstSet			= parentDescriptorSet;
		entry.descriptorCount	= 1;
		entry.dstArrayElement	= 0;
		entry.dstBinding		= binding.getBinding();
		switch( binding.getType() ) {
			case UniformLayout::Binding::Type::BLOCK: {
				if( binding.getUniformBuffer() ) {
					entry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					entry.pBufferInfo    = &(binding.getUniformBuffer()->getBufferInfo());
				}
			}
			break;

			case UniformLayout::Binding::Type::SAMPLER: {
				if( binding.getTexture() ) {
					entry.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					entry.pImageInfo     = &(binding.getTexture()->getImageInfo());
				}
			}
			break;

			case UniformLayout::Binding::Type::STORAGE_IMAGE: {
				if( binding.getTexture() ) {
					entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					entry.pImageInfo     = &(binding.getTexture()->getImageInfo());
				}
			}
			break;

			default: {
				// Probably means a new descriptor type got added and needs handling code.
				assert( false );
			}
			break;
		}

		result.push_back( entry );
		binding.clearDirty();
	}

	return result;
}

// -------------------------------------------------------------------------------------------------
// UniformSet
// -------------------------------------------------------------------------------------------------
UniformView::UniformView( const UniformLayout& layout, const UniformView::Options& options, vk::Device *device )
	: mDevice( device ), mOptions( options )
{
	// Copy sets
	for( const auto& srcSet : layout.getSets() ) {
		UniformView::SetRef set( new UniformView::Set( srcSet.getSet(), srcSet.getChangeFrequency() ) );
		mSets.push_back( set ); 
	}

	// Copy bindings
	const auto& srcBindings = layout.getBindings();
	for( const auto& srcBinding : srcBindings ) {
		auto it = std::find_if(
			std::begin( mSets ),
			std::end( mSets ),
			[srcBinding]( const UniformView::SetRef& elem ) -> bool {
				return srcBinding.getSet() == elem->getSetNumber();
			}
		);

		// There should never be a case in which a binding exists without a known set
		assert( std::end( mSets ) != it );

		// Create binding
		UniformView::Binding binding = UniformView::Binding( srcBinding );
		if( binding.isBlock() ) {
			if( options.getAllocateInitialBuffers() ) {
				vk::UniformBuffer::Format uniformBufferFormat = vk::UniformBuffer::Format();
				uniformBufferFormat.setTransientAllocation( mOptions.getTransientAllocation() );
				UniformBufferRef buffer = UniformBuffer::create( srcBinding.getBlock(), uniformBufferFormat, mDevice );
				binding.setUniformBuffer( buffer );
			}
			// Parse short names
			for( const auto& uniformVar : binding.getBlock().getUniforms() ) {
			}
		}
		// Get set
		auto& set = *it;
		// Add binding to set
		set->mBindings.push_back( binding );
	}

/*
	// Sort set by change frequency
	std::sort(
		std::begin( mSets ),
		std::end( mSets ),
		[]( const UniformSet::SetRef& a, const UniformSet::SetRef& b ) -> bool {
			return a->getChangeFrequency() < b->getChangeFrequency();
		}
	);
*/

	// Create DescriptorSetLayoutBindings
	for( auto& set : mSets ) {
		for( const auto& binding : set->mBindings ) {
			// Skip push constants
			if( UniformLayout::Binding::Type::PUSH_CONSTANTS == binding.getType() ) {
				continue;
			}
			// Proceed with the rest
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding			= binding.getBinding();
			layoutBinding.descriptorCount	= 1;
			layoutBinding.stageFlags		= binding.getShaderStages();
			layoutBinding.pImmutableSamplers = nullptr;
			switch( binding.getType() ) {
				case UniformLayout::Binding::Type::BLOCK: {
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				}
				break;

				case UniformLayout::Binding::Type::SAMPLER: {
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				}
				break;

				case UniformLayout::Binding::Type::STORAGE_IMAGE: {
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				}
				break;

				case UniformLayout::Binding::Type::STORAGE_BUFFER: {
					layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}
				break;
			}

			set->mDescriptorSetLayoutBindings.push_back( layoutBinding );
		}
	}

	// Cache - not as straight forward as it should be. To work around the implementation
	// differences, bindings *should* be densely populated and grouped by type. 
	for( auto& set : mSets ) {
		const auto& srcBindings = set->mDescriptorSetLayoutBindings;
		if( srcBindings.empty() ) {
			continue;
		}
		
		// Find the max binding number
		uint32_t maxBindingNumber = 0;
		for( const auto& srcBinding : srcBindings ) {
			maxBindingNumber = std::max<uint32_t>( maxBindingNumber, srcBinding.binding );
		}

		// Allocate descriptors
		const uint32_t bindingCount = maxBindingNumber + 1;
		std::vector<VkDescriptorSetLayoutBinding> dstBindings( bindingCount );

		// Populate the binding numbers
		for( uint32_t bindingNumber = 0; bindingNumber < bindingCount; ++bindingNumber ) {
			dstBindings[bindingNumber].binding = bindingNumber;
			dstBindings[bindingNumber].descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}

		// Copy the descriptor information
		for( auto& dstBinding : dstBindings ) {
			auto it = std::find_if( 
				std::begin( srcBindings ), std::end( srcBindings ),
				[dstBinding]( const VkDescriptorSetLayoutBinding& elem ) -> bool {
					return elem.binding == dstBinding.binding;
				}
			);

			if( std::end( srcBindings ) != it ) {
				const auto& srcBinding = *it;
				dstBinding.descriptorType     = srcBinding.descriptorType;
				dstBinding.descriptorCount    = srcBinding.descriptorCount;
				dstBinding.stageFlags         = srcBinding.stageFlags;
				dstBinding.pImmutableSamplers = srcBinding.pImmutableSamplers;
			}
		}

		// If entry N doesn't have a type use N-1's type...if N > 0
		for( size_t i = 1; i < dstBindings.size(); ++i ) {
			if( VK_DESCRIPTOR_TYPE_MAX_ENUM == dstBindings[i].descriptorType ) {
				dstBindings[i].descriptorType = dstBindings[i - 1].descriptorType;
			}
		}

		// Finally cache it
		mDescriptorSetLayoutBindings.push_back( dstBindings );
	}
}

UniformView::~UniformView()
{
}

UniformViewRef UniformView::create( const UniformLayout& layout, const UniformView::Options& options, vk::Device *device )
{
	device = ( nullptr != device ) ? device : vk::Context::getCurrent()->getDevice();
	UniformViewRef result = UniformViewRef( new UniformView( layout, options, device ) );
	return result;
}

std::vector<UniformLayout::PushConstant> UniformView::getPushConstants() const
{
	std::vector<UniformLayout::PushConstant> result;
	for( const auto& set : mSets ) {
		for( const auto& binding : set->mBindings ) {
			auto ranges = binding.getPushConstants();
			std::copy( std::begin( ranges ), std::end( ranges ), std::back_inserter( result ) );
		}
	}
	return result;
}

UniformView::Binding* UniformView::findBindingObject( const std::string& name, Binding::Type bindingType )
{
	UniformView::Binding* result = nullptr;

	for( const auto& set : mSets ) {
		auto it = std::find_if(
			std::begin( set->mBindings ),
			std::end( set->mBindings ),
			[&name]( const UniformView::Binding& elem ) -> bool {
				return ( elem.getName() == name );
			}
		);

		if( it != std::end( set->mBindings ) ) {
			// Only return if the found object's type is defined with in the requested's mask. 
			uint32_t bits = static_cast<uint32_t>( it->getType() );
			uint32_t mask = static_cast<uint32_t>( bindingType );
			if(  bits & mask ) {
				result = &(*it);
			}
		}

		if( nullptr != result ) {
			break;
		}
	}

	return result;
}

UniformView::BufferGroupRef UniformView::allocateBuffers() const
{
	std::vector<UniformView::BufferStoreRef> bufferStores;
	for( auto& set : mSets ) {
		for( auto& binding : set->mBindings ) {
			if( ! binding.isBlock() ) {
				continue;
			}
			vk::UniformBuffer::Format uniformBufferFormat = vk::UniformBuffer::Format();
			UniformBufferRef buffer = UniformBuffer::create( binding.getBlock(), uniformBufferFormat, mDevice );
			const uint32_t setNumber = set->getSetNumber();
			const uint32_t bindingNumber = binding.getBinding();
			UniformView::BufferStoreRef bindingStore( new UniformView::BufferStore( setNumber, bindingNumber, buffer ) );
			bufferStores.push_back( bindingStore );
		}
	}	
	return UniformView::BufferGroupRef( new UniformView::BufferGroup( bufferStores ) );
}

void UniformView::setBuffers(const UniformView::BufferGroupRef& bufferGroups)
{
	// Clear any buffers referenced
	for( auto& set : mSets ) {
		for( auto& binding : set->mBindings ) {
			if( ! binding.isBlock() ) {
				continue;
			}
			binding.mUniformBuffer.reset();
		}
	}

	const auto& bufferStores = bufferGroups->getBufferStores();
	for( const auto& bufferStore : bufferStores ) {
		// Find set
		const uint32_t setNumber = bufferStore->getSetNumber();
		auto setIt = std::find_if( std::begin( mSets ), std::end( mSets ),
			[setNumber]( const UniformView::SetRef& elem ) -> bool {
				return setNumber == elem->getSetNumber();
			}
		);
		// Skip if not found
		if( std::end( mSets ) == setIt ) {
			continue;
		}

		// Find binding
		const uint32_t bindingNumber = bufferStore->getBindingNumber();
		auto& bindings = (*setIt)->mBindings;
		auto bindingIt = std::find_if( std::begin( bindings ), std::end( bindings ),
			[bindingNumber]( const UniformView::Binding& elem ) -> bool {
				return bindingNumber == elem.getBinding();
			}
		);
		// Skip if not found
		if( std::end( bindings ) == bindingIt ) {
			continue;
		}

		// Set buffer
		bindingIt->setUniformBuffer( bufferStore->getUniformBuffer() );
	}
}

template <typename T>
void UniformView::updateUniform( const std::string& name, const T& value )
{
	// Parse out binding name
	std::vector<std::string> tokens = ci::split( name, "." );
	if( 2 != tokens.size() ) {
		std::string msg = "Invalid uniform name: " + name + ", must be in block.variable format";
		throw std::runtime_error( msg );
	}

	// Binding name
	const std::string& bindingName = tokens[0];

	/*
	// Parse out binding name
	std::vector<std::string> tokens = ci::split( name, "." );
	std::string bindingName = tokens[0];
	// If there's only 1 token, that means a short name was used and a binding name needs to be looked up.
	if( 1 == tokens.size() ) {
		auto it = mShortNameToBinding.find( tokens[0] );
		if( mShortNameToBinding.end() != it ) {
			bindingName = it->second;
		}
	}
	*/

	// Find binding and update
	UniformView::Binding* binding = this->findBindingObject( bindingName, Binding::Type::ANY );
	if( ( nullptr != binding ) && binding->isBlock() ) {
		binding->getUniformBuffer()->uniform( name, value );
	}
}

void UniformView::uniform( const std::string& name, const float value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const int32_t value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const uint32_t value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const bool value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const vec2& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const vec3& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const vec4& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const ivec2& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const ivec3& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const ivec4& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const uvec2& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const uvec3& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const uvec4& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const mat2& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const mat3& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const mat4& value )
{
	updateUniform( name, value );
}

void UniformView::uniform( const std::string& name, const TextureBaseRef& texture )
{
	auto bindingRef = findBindingObject( name, Binding::Type::SAMPLER );
	if( bindingRef ) {
		bindingRef->setTexture( texture );
	}
}

void UniformView::setDefaultUniformVars( vk::Context *context )
{
	for( auto& set : mSets ) {
		for( auto& binding : set->getBindings() ) {
			if( ! binding.isBlock() ) {
				continue;
			}
			context->setDefaultUniformVars( binding.getUniformBuffer() );
		}
	}
}

void UniformView::bufferPending( const vk::CommandBufferRef& cmdBuf, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask )
{
	bool addBarrier = false;
	for( auto& set : mSets ) {
		for( auto& binding : set->mBindings ) {
			if( ! binding.isBlock() ) {
				continue;
			}
			binding.getUniformBuffer()->transferPending();
			addBarrier = true;
		}
	}

	if( addBarrier ) {
		cmdBuf->pipelineBarrierGlobalMemory( vk::GlobalMemoryBarrierParams( srcAccessMask, dstAccessMask, srcStageMask, dstStageMask ) );
	}
}

void UniformView::echoValues( std::ostream& os )
{
}

}} // namespace cinder::vk