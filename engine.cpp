#include "engine.hpp"
#include "GLFW/glfw3.h"
#include "VkBootstrap.h"

#include "initializers.hpp"
#include "images.hpp"

#include <iostream>
#include <chrono>
#include <thread>

constexpr bool is_use_validation_layers = true;

namespace rstd {
	engine* loaded_engine = nullptr;
	engine& engine::get() { return *loaded_engine; }

	void engine::init() {
		assert(loaded_engine == nullptr);
		loaded_engine = this;

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(m_window_extent.width, m_window_extent.height, "rstd", nullptr, nullptr);
		if (!m_window) {
			fmt::print("Failed to create GLFW window\n");
			glfwTerminate();
			abort();
		}
		else {
			fmt::print("Success to create GLFW window\n");
		}

		init_vk();

		init_swapchain();

		init_commands();

		init_sync_structures();

		m_is_initialized = true;
	}

	void engine::clean() {
		if (m_is_initialized) {
			vkDeviceWaitIdle(m_device);

			for (int i = 0; i < FRAME_OVERLAP; i++) {
				vkDestroyCommandPool(m_device, m_frames[i].command_pool, nullptr);
				vkDestroyFence(m_device, m_frames[i].render_fence, nullptr);
				vkDestroySemaphore(m_device, m_frames[i].swapchain_sema, nullptr);
				vkDestroySemaphore(m_device, m_frames[i].render_sema, nullptr);
			}

			destroy_swapchain();

			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			vkDestroyDevice(m_device, nullptr);
			vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger, nullptr);
			vkDestroyInstance(m_instance, nullptr);
			glfwDestroyWindow(m_window);
		}

		glfwTerminate();
		loaded_engine = nullptr;
	}

	void engine::draw() {
		VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().render_fence, true, 100000000));
		VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().render_fence));

		uint32_t swapchain_image_index;
		VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, get_current_frame().swapchain_sema, nullptr, &swapchain_image_index));

		VkCommandBuffer cmd = get_current_frame().command_buffer;
		VK_CHECK(vkResetCommandBuffer(cmd, 0));
		
		VkCommandBufferBeginInfo  cmd_begin_info = command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));
		transition_image(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		
		VkClearColorValue clear_color;
		float flash = std::abs(std::sin(m_frame_number / 120.f));
		clear_color = { { 0.0f, 0.0f, flash, 1.0f } };

		VkImageSubresourceRange clear_range = init_image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
		vkCmdClearColorImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &clear_range);
		
		transition_image(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmd_info = init_command_buffer_submit_info(cmd);
		VkSemaphoreSubmitInfo wait_info = init_semaphore_submit_info(
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
			get_current_frame().swapchain_sema
		);
		VkSemaphoreSubmitInfo signal_info = init_semaphore_submit_info(
			VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
			get_current_frame().render_sema
		);
		VkSubmitInfo2 submit = init_submit_info(&cmd_info, &signal_info, &wait_info);

		VK_CHECK(vkQueueSubmit2(m_graphics_queue, 1, &submit, get_current_frame().render_fence));

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pNext = nullptr;
		present_info.pSwapchains = &m_swapchain;
		present_info.swapchainCount = 1;

		present_info.pWaitSemaphores = &get_current_frame().render_sema;
		present_info.waitSemaphoreCount = 1;
		
		present_info.pImageIndices = &swapchain_image_index;

		VK_CHECK(vkQueuePresentKHR(m_graphics_queue, &present_info));
		m_frame_number++;
	}

	void engine::run() {
		while (!glfwWindowShouldClose(m_window)) {
			glfwPollEvents();
			draw();
		}
	}
	void engine::init_vk() {
		vkb::InstanceBuilder builder;
		auto inst_ret = builder.set_app_name("rstd")
			.request_validation_layers(is_use_validation_layers)
			.use_default_debug_messenger()
			.require_api_version(1, 3, 0)
			.build();

		vkb::Instance vkb_inst = inst_ret.value();
		m_instance = vkb_inst.instance;
		m_debug_messenger = vkb_inst.debug_messenger;

		VK_CHECK(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface));

		VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features13.dynamicRendering = true;
		features13.synchronization2 = true;

		//vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		vkb::PhysicalDevice physical_device = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features13)
			.set_required_features_12(features12)
			.set_surface(m_surface)
			.select()
			.value();

		//create the final vulkan device
		vkb::DeviceBuilder device_builder{ physical_device };
		vkb::Device vkb_device = device_builder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		m_device = vkb_device.device;
		m_gpu = physical_device.physical_device;

		m_graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
		m_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
	}

	void engine::init_swapchain() {
		create_swapchain(m_window_extent.width, m_window_extent.height);
	}
	void engine::init_commands() {
		VkCommandPoolCreateInfo command_pool_info = init_command_pool_create_info(m_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_frames[i].command_pool););
			VkCommandBufferAllocateInfo command_buffer_allocate_info = init_command_buffer_allocate_info(m_frames[i].command_pool, 1);
			VK_CHECK(vkAllocateCommandBuffers(m_device, &command_buffer_allocate_info, &m_frames[i].command_buffer));
		}
	}
	void engine::init_sync_structures() {
		VkFenceCreateInfo fence_create_info = init_fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphore_create_info = init_semaphore_create_info(0);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateFence(m_device, &fence_create_info, nullptr, &m_frames[i].render_fence));
			VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].swapchain_sema));
			VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].render_sema));
		}
	}

	void engine::create_swapchain(uint32_t width, uint32_t height) {
		vkb::SwapchainBuilder swapchain_builder{ m_gpu, m_device, m_surface };
		m_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkb_swapchain = swapchain_builder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_swapchain_extent = vkb_swapchain.extent;
		m_swapchain = vkb_swapchain.swapchain;
		m_swapchain_images = vkb_swapchain.get_images().value();
		m_swapchain_image_views = vkb_swapchain.get_image_views().value();
	}

	void engine::destroy_swapchain() {
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		for (int i = 0; i < m_swapchain_image_views.size(); i++) {
			vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
		}
	}
}

