#pragma once

#include "types.hpp"

struct GLFWwindow;

namespace rstd {
	constexpr unsigned int FRAME_OVERLAP = 2;

	struct frame_data {
		VkCommandPool command_pool;
		VkCommandBuffer command_buffer;
		VkSemaphore swapchain_sema, render_sema;
		VkFence render_fence;
	};

	class engine {
	public:
		// main functions 
		static engine& get();

		void init();
		void clean();
		void draw();
		void run();

		frame_data m_frames[FRAME_OVERLAP];
		frame_data& get_current_frame() { return m_frames[m_frame_number % FRAME_OVERLAP]; }

		bool m_is_initialized{ false };
		int m_frame_number{ 0 };
		bool m_stop_rendering{ false };
		VkExtent2D m_window_extent{ 1280, 720 };

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkPhysicalDevice m_gpu;
		VkDevice m_device;
		VkSurfaceKHR m_surface;

		VkSwapchainKHR m_swapchain;
		VkFormat m_swapchain_image_format;

		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		VkExtent2D m_swapchain_extent;

		VkQueue m_graphics_queue;
		uint32_t m_graphics_queue_family;

	private:
		void init_vk();
		void init_swapchain();
		void init_commands();
		void init_sync_structures();

		void create_swapchain(uint32_t width, uint32_t height);
		void destroy_swapchain();


		GLFWwindow* m_window{ nullptr };
	};
}