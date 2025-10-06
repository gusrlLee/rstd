#pragma once
#include "types.hpp"

namespace rstd {
	VkCommandPoolCreateInfo init_command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags) {
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.queueFamilyIndex = queue_family_index;
		info.flags = flags;
		return info;
	}

	VkCommandBufferAllocateInfo init_command_buffer_allocate_info(VkCommandPool pool, uint32_t count) {
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext = nullptr;
		info.commandPool = pool;
		info.commandBufferCount = count;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		return info;
	}

	VkFenceCreateInfo init_fence_create_info(VkFenceCreateFlags flags /*= 0*/) {
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = flags;
		return info;
	}

	VkSemaphoreCreateInfo init_semaphore_create_info(VkSemaphoreCreateFlags flags /*= 0*/) {
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = flags;
		return info;
	}

	VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext = nullptr;
		info.pInheritanceInfo = nullptr;
		info.flags = flags;
		return info;
	}

	VkImageSubresourceRange init_image_subresource_range(VkImageAspectFlags aspect_mask) {
		VkImageSubresourceRange sub_image{};
		sub_image.aspectMask = aspect_mask;
		sub_image.baseMipLevel = 0;
		sub_image.levelCount = VK_REMAINING_MIP_LEVELS;
		sub_image.baseArrayLayer = 0;
		sub_image.layerCount = VK_REMAINING_ARRAY_LAYERS;

		return sub_image;
	}

	VkSemaphoreSubmitInfo init_semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore) {
		VkSemaphoreSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.semaphore = semaphore;
		submitInfo.stageMask = stage_mask;
		submitInfo.deviceIndex = 0;
		submitInfo.value = 1;

		return submitInfo;
	}

	VkCommandBufferSubmitInfo init_command_buffer_submit_info(VkCommandBuffer cmd) {
		VkCommandBufferSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		info.pNext = nullptr;
		info.commandBuffer = cmd;
		info.deviceMask = 0;

		return info;
	}

	VkSubmitInfo2 init_submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signal_sema_info, VkSemaphoreSubmitInfo* wait_sema_info) {
		VkSubmitInfo2 info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.pNext = nullptr;

		info.waitSemaphoreInfoCount = wait_sema_info == nullptr ? 0 : 1;
		info.pWaitSemaphoreInfos = wait_sema_info;

		info.signalSemaphoreInfoCount = signal_sema_info == nullptr ? 0 : 1;
		info.pSignalSemaphoreInfos = signal_sema_info;

		info.commandBufferInfoCount = 1;
		info.pCommandBufferInfos = cmd;

		return info;
	}
}