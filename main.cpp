// https://github.com/KhronosGroup/Vulkan-Tutorial/blob/main/attachments/16_frames_in_flight.cpp

#include <algorithm>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS 
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t kWidth = 800;
const uint32_t kHeight = 600;


class Application {
public:
	void run()
	{
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	}

private:
	uint32_t width_ = 800;
	uint32_t height_ = 600;

#ifdef _DEBUG
	const bool enable_validation_layers = true;
#else 
	const bool enable_validation_layers = false;
#endif

	GLFWwindow* window_ = nullptr;
	vk::raii::Context context_;
	vk::raii::Instance instance_ = nullptr;
	vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;

	vk::raii::SurfaceKHR surface_ = nullptr;

	vk::raii::PhysicalDevice physical_device_ = nullptr;
	vk::raii::Device device_ = nullptr;
	
	uint32_t queue_index_ = ~0;
	vk::raii::Queue queue_ = nullptr;

	vk::raii::SwapchainKHR swap_chain_ = nullptr;
	std::vector<vk::Image> swap_chain_images_;
	vk::SurfaceFormatKHR swap_chain_surface_format_;
	vk::Extent2D swap_chain_extent_;
	std::vector<vk::raii::ImageView> swap_chain_image_views_;

	vk::raii::PipelineLayout pipeline_layout_ = nullptr;
	vk::Pipeline graphics_pipeline_ = nullptr;

	vk::raii::CommandPool command_pool_ = nullptr;
	std::vector<vk::raii::CommandBuffer> command_buffer_;

	std::vector<vk::raii::Semaphore> present_complete_semaphores_;
	std::vector<vk::raii::Semaphore> render_finished_semaphores_;
	std::vector<vk::raii::Fence> in_flight_fences_;

	uint32_t semaphore_index_ = 0;
	uint32_t current_frame_ = 0;
	const int kMaxFrameInFlight = 2;

	std::vector<const char*> required_device_extensions_ = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName
	};

	const std::vector<const char*> validation_layers_ = {
		"VK_LAYER_KHRONOS_validation"
	};

	void InitWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window_ = glfwCreateWindow(width_, height_, "Vulkan", nullptr, nullptr);
		if (!window_) 
		{
			glfwTerminate();
			throw std::runtime_error("[SYSTEM ERROR] Failed to create GLFW window");
		}
	}

	void InitVulkan()
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(window_))
		{
			glfwPollEvents();
		}

		device_.waitIdle();
	}

	void Cleanup()
	{
		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void CreateInstance()
	{
		vk::ApplicationInfo app_info{
									.pApplicationName = "Vulkan application",
									.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
									.pEngineName = "No Engine",
									.engineVersion = VK_MAKE_VERSION(1, 0, 0),
									.apiVersion = VK_API_VERSION_1_4};

		// Get the required extensions layer 
		std::vector<const char*> required_layers;
		if (enable_validation_layers)
		{
			required_layers.assign(validation_layers_.begin(), validation_layers_.end());
		}

		// Check if the required layers are supported by the Vulkan implementation
		auto layer_properties = context_.enumerateInstanceLayerProperties();
		for (auto const& layer_property : layer_properties)
		{
			std::cout << "Available layer: " << layer_property.layerName << std::endl;
		}

		for (auto const& required_layer : required_layers) 
		{
			if (std::ranges::none_of(layer_properties, 
				[required_layer](auto const& layer_property) {
					return strcmp(layer_property.layerName, required_layer) == 0; })
				)
			{
				throw std::runtime_error("Required layer not supported: " + std::string(required_layer));
			}
		}

		// Get the reqruied extensions
		auto required_extensions = GetRequiredExtensions();

		auto extension_properties = context_.enumerateInstanceExtensionProperties();
		for (auto const& required_extension : required_extensions)
		{
			if (std::ranges::none_of(extension_properties, [required_extension](auto const& extension_property) {return strcmp(extension_property.extensionName, required_extension) == 0; }))
			{
				throw std::runtime_error("Required layer not supported: " + std::string(required_extension));
			}
		}

		vk::InstanceCreateInfo create_info
		{
			.pApplicationInfo = &app_info,
			.enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
			.ppEnabledLayerNames = required_layers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
			.ppEnabledExtensionNames = required_extensions.data()
		};

		instance_ = vk::raii::Instance(context_, create_info);
	}

	void SetupDebugMessenger()
	{
		if (!enable_validation_layers)
			return;

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT  debugUtilsMessengerCreateInfoEXT{
			 .messageSeverity = severityFlags,
			 .messageType = messageTypeFlags,
			 .pfnUserCallback = &DebugCallback};
		debug_messenger_ = instance_.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void CreateSurface()
	{
		VkSurfaceKHR surface;
		if (glfwCreateWindowSurface(*instance_, window_, nullptr, &surface) != 0)
		{
			throw std::runtime_error("failed to create window surface!");
		}
		surface_ = vk::raii::SurfaceKHR(instance_, surface);
	}

	void PickPhysicalDevice()
	{
		std::vector<vk::raii::PhysicalDevice> devices = instance_.enumeratePhysicalDevices();
		const auto dev_iter = std::ranges::find_if(
			devices,
			[&](auto const& device) {
				bool supports_vulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

				auto queue_families = device.getQueueFamilyProperties();
				bool supports_graphics = std::ranges::any_of(
					queue_families,
					[](auto const& qfp) {
						return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
					}
				);

				auto available_device_extensions = device.enumerateDeviceExtensionProperties();
				bool supports_all_required_extensions = 
					std::ranges::all_of(required_device_extensions_,
						[&available_device_extensions](auto const& required_device_extension) {
							return std::ranges::any_of(available_device_extensions,
								[required_device_extension](auto const& avaliable_device_extension) { return strcmp(avaliable_device_extension.extensionName, required_device_extension) == 0; });
						});

				auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2,
					vk::PhysicalDeviceVulkan11Features,
					vk::PhysicalDeviceVulkan13Features,
					vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

				bool support_required_features = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
					features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
					features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
					features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

				return supports_vulkan1_3 && supports_graphics && supports_all_required_extensions && support_required_features;
			});
		if (dev_iter != devices.end())
		{
			physical_device_ = *dev_iter;
		}
		else
		{
			throw std::runtime_error("Failed to find a suitable GPU");
		}
	}

	void CreateLogicalDevice()
	{
		std::vector<vk::QueueFamilyProperties> qfp = physical_device_.getQueueFamilyProperties();
		for (uint32_t qfp_idx = 0; qfp_idx < qfp.size(); qfp_idx++)
		{
			if ((qfp[qfp_idx].queueFlags & vk::QueueFlagBits::eGraphics) && physical_device_.getSurfaceSupportKHR(qfp_idx, *surface_))
			{
				queue_index_ = qfp_idx;
				break;
			}
		}

		if (queue_index_ == ~0)
		{
			throw std::runtime_error("Could no find a queue for graphics and present -> terminating");
		}

		vk::StructureChain<vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> features_chain = {
				{},                                                          // vk::PhysicalDeviceFeatures2
				{.shaderDrawParameters = true},                              // vk::PhysicalDeviceVulkan11Features
				{.synchronization2 = true, .dynamicRendering = true},        // vk::PhysicalDeviceVulkan13Features
				{.extendedDynamicState = true}                               // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
		};

		float queue_priority = 0.5f;
		vk::DeviceQueueCreateInfo device_queue_create_info{ .queueFamilyIndex = queue_index_, .queueCount = 1, .pQueuePriorities = &queue_priority };
		vk::DeviceCreateInfo      device_create_info { 
			.pNext = &features_chain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &device_queue_create_info,
			.enabledExtensionCount = static_cast<uint32_t>(required_device_extensions_.size()),
			.ppEnabledExtensionNames = required_device_extensions_.data() 
		};

		device_ = vk::raii::Device(physical_device_, device_create_info);
		queue_ = vk::raii::Queue(device_, queue_index_, 0);
	}

	void CreateSwapChain()
	{
	
	}


	std::vector<const char*> GetRequiredExtensions()
	{
		uint32_t glfw_extension_count = 0;
		auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
		if (enable_validation_layers) 
		{
			extensions.push_back(vk::EXTDebugUtilsExtensionName);
		}
		return extensions;
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*)
	{
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{
			std::cerr << "validation layer: type " << to_string(type) << " msg: " << callback_data->pMessage << std::endl;
		}

		return vk::False;
	}
};

int main()
{
	try
	{
		Application app;
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}