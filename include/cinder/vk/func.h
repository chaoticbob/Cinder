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

#include "cinder/vk/platform.h"

namespace cinder { namespace vk {

class CommandBuffer;
class Device;
class Fence;
class IndexBuffer;
class Pipeline;
class PipelineLayout;
class Semaphore;
class VertexBuffer;
using CommandBufferRef = std::shared_ptr<CommandBuffer>;
using DeviceRef = std::shared_ptr<Device>;
using FenceRef = std::shared_ptr<Fence>;
using IndexBufferRef = std::shared_ptr<IndexBuffer>;
using PipelineRef = std::shared_ptr<Pipeline>;
using PipelineLayoutRef = std::shared_ptr<PipelineLayout>;
using SemaphoreRef = std::shared_ptr<Semaphore>;
using VertexBufferRef = std::shared_ptr<VertexBuffer>;

} } // namespace cinder::vk

VkResult	vkCreateFence( const ci::vk::Device* device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence );
void		vkDestroyFence( const ci::vk::Device* device, VkFence fence, const VkAllocationCallbacks* pAllocator );
VkResult	vkResetFences( const ci::vk::Device* device, uint32_t fenceCount, const VkFence* pFences );
VkResult	vkGetFenceStatus( const ci::vk::Device* device, VkFence fence );
VkResult	vkWaitForFences( const ci::vk::Device* device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout );
VkResult	vkWaitForFences( const ci::vk::Device* device, const std::vector<ci::vk::FenceRef>& fences, VkBool32 waitAll, uint64_t timeout );

VkResult	vkCreateSemaphore( const ci::vk::Device* device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore );
void		vkDestroySemaphore( const ci::vk::Device* device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator );

VkResult	vkCreateComputePipelines( const ci::vk::Device* device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines );

void		vkCmdBindPipeline( const ci::vk::CommandBufferRef& commandBufferr, VkPipelineBindPoint  pipelineBindPoint, VkPipeline pipeline );
void		vkCmdBindPipeline( const ci::vk::CommandBufferRef& commandBufferr, VkPipelineBindPoint  pipelineBindPoint, const ci::vk::PipelineRef& pipeline );
void		vkCmdBindDescriptorSets( const ci::vk::CommandBufferRef& commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets );
void		vkCmdBindDescriptorSets( const ci::vk::CommandBufferRef& commandBuffer, VkPipelineBindPoint pipelineBindPoint, const ci::vk::PipelineLayoutRef& layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets );
void		vkCmdBindIndexBuffer( const ci::vk::CommandBufferRef& commandBuffer, const ci::vk::IndexBufferRef& buffer, VkDeviceSize offset = 0 );
void		vkCmdBindVertexBuffers( const ci::vk::CommandBufferRef& commandBuffer, const ci::vk::VertexBufferRef& buffer, VkDeviceSize offset = 0 );
void		vkCmdBindVertexBuffers( const ci::vk::CommandBufferRef& commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets );
void		vkCmdDraw( const ci::vk::CommandBufferRef& commandBuffer, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0 );
void		vkCmdDrawIndexed( const ci::vk::CommandBufferRef& commandBuffer, uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0 );
void		vkCmdPushConstants( const ci::vk::CommandBufferRef& commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues );
void		vkCmdPushConstants( const ci::vk::CommandBufferRef& commandBuffer, const ci::vk::PipelineLayoutRef& layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues );