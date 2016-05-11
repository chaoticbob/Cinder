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

#pragma once

#include "cinder/vk/UniformLayout.h"

namespace cinder { namespace vk {

//! \class UniformSet
//!
//!
class UniformView { 
public:

	//! \class Binding
	//!
	//!
	class Binding : public UniformLayout::Binding {
	public:
		Binding() {}
		Binding( const UniformLayout::Binding& binding )
			: UniformLayout::Binding( binding ) {}
		virtual ~Binding() {}

		const UniformBufferRef&		getUniformBuffer() const { return mUniformBuffer; }
		void						setUniformBuffer( const UniformBufferRef& buffer );

	private:
		UniformBufferRef			mUniformBuffer;
		friend class UniformView;
	};

	//! \class Set
	//!
	//!
	class Set {
	public:

		Set( uint32_t setNumber, uint32_t changeFrequency ) : mSetNumber( setNumber ), mChangeFrequency( changeFrequency ) {}
		virtual ~Set() {}

		uint32_t											getSetNumber() const { return mSetNumber; }
		uint32_t											getChangeFrequency() const { return mChangeFrequency; }
		const std::vector<Binding>&							getBindings() const { return mBindings; }
		const std::vector<VkDescriptorSetLayoutBinding>&	getDescriptorSetlayoutBindings() const { return mDescriptorSetLayoutBindings; } 

		std::vector<VkWriteDescriptorSet>					getBindingUpdates( VkDescriptorSet parentDescriptorSet );

	private:
		uint32_t											mSetNumber = DEFAULT_SET;
		uint32_t											mChangeFrequency = CHANGES_DONTCARE;
		std::vector<Binding>								mBindings;
		std::vector<VkDescriptorSetLayoutBinding>			mDescriptorSetLayoutBindings;
		friend class UniformView;
	};
	
	using SetRef = std::shared_ptr<UniformView::Set>;

	// ---------------------------------------------------------------------------------------------

	class BufferStore {
	public:
		BufferStore( uint32_t setNumber, uint32_t bindingNumber, const UniformBufferRef& uniformBuffer )
			: mSetNumber( setNumber ), mBindingNumber( bindingNumber ), mUniformBuffer( uniformBuffer ) {}
		virtual ~BufferStore() {}
		uint32_t				getSetNumber() const { return mSetNumber; }
		uint32_t				getBindingNumber() const { return mBindingNumber; }
		const UniformBufferRef&	getUniformBuffer() const { return mUniformBuffer; }
	private:
		uint32_t				mSetNumber = UINT32_MAX;
		uint32_t				mBindingNumber = UINT32_MAX;
		UniformBufferRef		mUniformBuffer;
		friend class UniformView;
	};

	using BufferStoreRef = std::shared_ptr<UniformView::BufferStore>;

	class BufferGroup {
	public:
		BufferGroup( std::vector<UniformView::BufferStoreRef> bufferStores = std::vector<UniformView::BufferStoreRef>() ) 
			: mBufferStores( bufferStores ) {}
		virtual ~BufferGroup() {}
		size_t											getBufferStoreCount() const { return mBufferStores.size(); }
		const UniformView::BufferStoreRef&				getBufferStore( size_t n )const { return mBufferStores[n]; }
		const std::vector<UniformView::BufferStoreRef>&	getBufferStores() const { return mBufferStores; }
	private:
		std::vector<UniformView::BufferStoreRef>		mBufferStores;
		void addBufferStore( const UniformView::BufferStoreRef& bufferStore ) { mBufferStores.push_back( bufferStore ); }
		friend class UniformView;
	};

	using BufferGroupRef = std::shared_ptr<BufferGroup>;

	// ---------------------------------------------------------------------------------------------

	//! \class Options
	//!
	//!
	class Options {
	public:
		Options() {}
		virtual ~Options() {}
		Options&	setAllocateInitialBuffers( bool value = true ) { mAllocateInitialBuffers = value; return *this; }
		bool		getAllocateInitialBuffers() const { return mAllocateInitialBuffers; }
		Options&	setTransientAllocation( bool value = true ) { mTransientAllocation = value; if( value ) { mAllocateInitialBuffers = true; } return *this; }
		bool		getTransientAllocation() const { return mTransientAllocation; }
	private:
		bool		mAllocateInitialBuffers = true;
		bool		mTransientAllocation = false;
		friend class UniformView;
	};

	// ---------------------------------------------------------------------------------------------

	UniformView( const UniformLayout& layout, const UniformView::Options& options, vk::Device *device );
	virtual ~UniformView();

	static UniformViewRef				create( const UniformLayout& layout, const UniformView::Options& options = UniformView::Options(), vk::Device *device = nullptr );

	std::vector<UniformLayout::PushConstant>						getPushConstants() const;

	const std::vector<UniformView::SetRef>&							getSets() const { return mSets; }
	const std::vector<std::vector<VkDescriptorSetLayoutBinding>>&	getDescriptorSetLayoutBindings() const { return mDescriptorSetLayoutBindings; }

	UniformView::BufferGroupRef			allocateBuffers() const;
	void								setBuffers( const UniformView::BufferGroupRef& bufferGroups );

	void								uniform( const std::string& name, const float    value );
	void								uniform( const std::string& name, const int32_t  value );
	void								uniform( const std::string& name, const uint32_t value );
	void								uniform( const std::string& name, const bool     value );
	void								uniform( const std::string& name, const vec2&    value );
	void								uniform( const std::string& name, const vec3&    value );
	void								uniform( const std::string& name, const vec4&    value );
	void								uniform( const std::string& name, const ivec2&   value );
	void								uniform( const std::string& name, const ivec3&   value );
	void								uniform( const std::string& name, const ivec4&   value );
	void								uniform( const std::string& name, const uvec2&   value );
	void								uniform( const std::string& name, const uvec3&   value );
	void								uniform( const std::string& name, const uvec4&   value );
	void								uniform( const std::string& name, const mat2&    value );
	void								uniform( const std::string& name, const mat3&    value );
	void								uniform( const std::string& name, const mat4&    value );
	void								uniform( const std::string& name, const TextureBaseRef& texture );

	void								setDefaultUniformVars( vk::Context *context );
	void								bufferPending( const vk::CommandBufferRef& cmdBuf, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask );

	void								echoValues( std::ostream& os );

private:
	vk::Device							*mDevice = nullptr;

	UniformView::Options				mOptions;
	std::map<std::string, std::string>	mShortNameToBinding;

	std::vector<UniformView::SetRef>						mSets;
	std::vector<std::vector<VkDescriptorSetLayoutBinding>>	mDescriptorSetLayoutBindings;

	Binding*						findBindingObject( const std::string& name, Binding::Type bindingType );

	template <typename T>
	void updateUniform( const std::string& name, const T& value );
};

}} // namespace cinder::vk