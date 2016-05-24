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

#include "cinder/vk/func.h"
#include "cinder/vk/CommandBuffer.h"
#include "cinder/vk/Device.h"
#include "cinder/vk/IndexBuffer.h"
#include "cinder/vk/Pipeline.h"
#include "cinder/vk/Sync.h"
#include "cinder/vk/VertexBuffer.h"
using namespace ci;

VkResult vkCreateFence( const ci::vk::Device* device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence )
{
	return vkCreateFence( device->vk(), pCreateInfo, pAllocator, pFence );
}

void vkDestroyFence( const ci::vk::Device* device, VkFence fence, const VkAllocationCallbacks* pAllocator )
{
	vkDestroyFence( device->vk(), fence, pAllocator );
}

VkResult vkResetFences( const ci::vk::Device* device, uint32_t fenceCount, const VkFence* pFences )
{
	return vkResetFences( device->vk(), fenceCount, pFences );
}

VkResult vkGetFenceStatus( const ci::vk::Device* device, VkFence fence )
{
	return vkGetFenceStatus( device->vk(), fence );
}

VkResult vkWaitForFences( const ci::vk::Device* device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout )
{
	return vkWaitForFences( device->vk(), fenceCount, pFences, waitAll, timeout );
}

VkResult vkWaitForFences( const ci::vk::Device* device, const std::vector<ci::vk::FenceRef>& fences, VkBool32 waitAll, uint64_t timeout )
{
	std::vector<VkFence> fencesVk;
	for( const auto& fence : fences ) {
		fencesVk.push_back( fence->getVkObject() );
	}

	return vkWaitForFences( device, static_cast<uint32_t>( fencesVk.size() ), fencesVk.data(), waitAll, timeout );
}

VkResult vkCreateSemaphore( const ci::vk::Device* device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore )
{
	return vkCreateSemaphore( device->vk(), pCreateInfo, pAllocator, pSemaphore );
}

void vkDestroySemaphore( const ci::vk::Device* device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator )
{
	vkDestroySemaphore( device->vk(), semaphore, pAllocator );
}

VkResult vkCreateComputePipelines( const ci::vk::Device* device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines )
{
	return vkCreateComputePipelines( device->vk(), pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines );
}

void vkCmdBindPipeline( const ci::vk::CommandBufferRef& commandBufferr, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline )
{
	vkCmdBindPipeline( commandBufferr->vk(), pipelineBindPoint, pipeline );
}

void vkCmdBindPipeline( const ci::vk::CommandBufferRef& commandBufferr, VkPipelineBindPoint  pipelineBindPoint, const ci::vk::PipelineRef& pipeline )
{
	vkCmdBindPipeline( commandBufferr->vk(), pipelineBindPoint, pipeline->vk() );
}

void vkCmdBindDescriptorSets(const vk::CommandBufferRef& commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
	vkCmdBindDescriptorSets( commandBuffer->vk(), pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets );
}

void vkCmdBindDescriptorSets( const ci::vk::CommandBufferRef& commandBuffer, VkPipelineBindPoint pipelineBindPoint, const ci::vk::PipelineLayoutRef& layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets )
{
	vkCmdBindDescriptorSets( commandBuffer->vk(), pipelineBindPoint, layout->vk(), firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets );
}

void vkCmdBindIndexBuffer( const ci::vk::CommandBufferRef& commandBuffer, const ci::vk::IndexBufferRef& buffer, VkDeviceSize offset )
{
	vkCmdBindIndexBuffer( commandBuffer->vk(), buffer->vk(), offset, buffer->getIndexType() );
}

void vkCmdBindVertexBuffers(const ci::vk::CommandBufferRef& commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const ci::vk::VertexBufferRef& buffer, VkDeviceSize offset )
{
	std::vector<VkBuffer> buffers = { buffer->vk() };
	std::vector<VkDeviceSize> offsets = { offset };
	vkCmdBindVertexBuffers( commandBuffer->vk(), 0, 1, buffers.data(), offsets.data() );
}

void vkCmdBindVertexBuffers(const ci::vk::CommandBufferRef& commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets)
{
	vkCmdBindVertexBuffers( commandBuffer->vk(), firstBinding, bindingCount, buffers.data(), offsets.data() );
}

void vkCmdDraw( const ci::vk::CommandBufferRef& commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance )
{
	vkCmdDraw( commandBuffer->vk(), vertexCount, instanceCount, firstVertex, firstInstance );
}

void vkCmdDrawIndexed(const ci::vk::CommandBufferRef& commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	vkCmdDrawIndexed( commandBuffer->vk(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
}

void vkCmdPushConstants( const ci::vk::CommandBufferRef& commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues )
{
	vkCmdPushConstants( commandBuffer->vk(), layout, stageFlags, offset, size, pValues );
}

void vkCmdPushConstants(const ci::vk::CommandBufferRef& commandBuffer, const ci::vk::PipelineLayoutRef& layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues)
{
	vkCmdPushConstants( commandBuffer->vk(), layout->vk(), stageFlags, offset, size, pValues );
}
