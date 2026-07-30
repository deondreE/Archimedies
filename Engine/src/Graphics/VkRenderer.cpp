#include "archpch.h"
#include "VkRenderer.h"

namespace Engine::Graphics {

	static VkContext* s_ActiveContext = nullptr;

	VkContext& GetVkContext()
	{
		ARCH_ASSERT(s_ActiveContext != nullptr && "No active VkContext - was VkRenderer::Init() called?");
		return *s_ActiveContext;
	}

	namespace 
	{
		struct QueueFamilyIndices
		{
			std::optional<u32> GraphicsFamily;

			bool IsComplete() const { return GraphicsFamily.has_value(); }
		};

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
		{
			QueueFamilyIndices indices;
			// There has got to be a better way to do this, why call 
			// vkGetPhysicalDeviceQueueFamilyProperties twice, once for number other for props?
			u32 queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
			
			for (u32 i = 0; i < queueFamilyCount; ++i) {
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					indices.GraphicsFamily = i;
				}
				if (indices.IsComplete())
				{
					break;
				}
			}
			return indices;
		}
	}

	VkRenderer::~VkRenderer()
	{
		Shutdown();
	}

	void VkRenderer::Init(void* windowHandle, void* instance) {
		// Window handle is expected to be a platform window handle.
		// @Todo: Expected to be crossplatform at some point.

		CreateInstance((HINSTANCE)instance, (HWND)windowHandle);
		SelectPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapchain();

		s_ActiveContext = &_Context;

		QueueFamilyIndices indices = FindQueueFamilies(_Context.PhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = indices.GraphicsFamily.value()
		};

		if (vkCreateCommandPool(_Context.LogicalDevice, &poolInfo, nullptr, &_CommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool!");
		}
		_CommandBuffers.resize(GxSyncObjects::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = _CommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<u32>(_CommandBuffers.size())
		};

		if (vkAllocateCommandBuffers(_Context.LogicalDevice, &allocInfo, _CommandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffers!");
		}

		VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 
		};

		VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		for (int i = 0; i < GxSyncObjects::MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(_Context.LogicalDevice, &semaphoreInfo, nullptr, &_SyncObjects.ImageAvailibleSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(_Context.LogicalDevice, &semaphoreInfo, nullptr, &_SyncObjects.RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(_Context.LogicalDevice, &fenceInfo, nullptr, &_SyncObjects.InFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create synchronization objects for a frame!");
			}
		}
	}

	void VkRenderer::Shutdown()
	{
		if (_Context.LogicalDevice == VK_NULL) return;

		vkDeviceWaitIdle(_Context.LogicalDevice);

		for (int i = 0; i < GxSyncObjects::MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (_SyncObjects.RenderFinishedSemaphores[i] != VK_NULL) vkDestroySemaphore(_Context.LogicalDevice, _SyncObjects.RenderFinishedSemaphores[i], nullptr);
			if (_SyncObjects.ImageAvailibleSemaphores[i] != VK_NULL) vkDestroySemaphore(_Context.LogicalDevice, _SyncObjects.ImageAvailibleSemaphores[i], nullptr);
			if (_SyncObjects.InFlightFences[i] != VK_NULL) vkDestroyFence(_Context.LogicalDevice, _SyncObjects.InFlightFences[i], nullptr);
		}
		
		if (_CommandPool != VK_NULL)
		{
			vkDestroyCommandPool(_Context.LogicalDevice, _CommandPool, nullptr);
			_CommandPool = VK_NULL;
		}

		if (_Swapchain != VK_NULL)
		{
			vkDestroySwapchainKHR(_Context.LogicalDevice, _Swapchain, nullptr);
			_Swapchain = VK_NULL;
		}

		vkDestroyDevice(_Context.LogicalDevice, nullptr);
		_Context.LogicalDevice = VK_NULL;

		if (_Context.Surface != VK_NULL)
		{
			vkDestroySurfaceKHR(_Context.Instance, _Context.Surface, nullptr);
			_Context.Surface = VK_NULL;
		}

		if (_Context.Instance != VK_NULL)
		{
			vkDestroyInstance(_Context.Instance, nullptr);
			_Context.Instance = VK_NULL;
		}

		if (s_ActiveContext == &_Context)
		{
			s_ActiveContext = nullptr;
		}
	}

	void VkRenderer::BeginFrame()
	{
		vkWaitForFences(_Context.LogicalDevice, 1, &_SyncObjects.InFlightFences[_SyncObjects.CurrentFrame], VK_TRUE, UINT64_MAX);

		u32 imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(
			_Context.LogicalDevice,
			_Swapchain,
			UINT64_MAX,
			_SyncObjects.ImageAvailibleSemaphores[_SyncObjects.CurrentFrame],
			VK_NULL,
			&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// @Todo: Recreate swapchain.
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swapchain image!");
		}

		_CurrentImageIndex = imageIndex;
		
		vkResetFences(_Context.LogicalDevice, 1, &_SyncObjects.InFlightFences[_SyncObjects.CurrentFrame]);

		VkCommandBuffer commandBuffer = _CommandBuffers[_SyncObjects.CurrentFrame]; 
		vkResetCommandBuffer(commandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to begin recording command buffer!");
		}
	}

	void VkRenderer::EndFrame()
	{
		VkCommandBuffer commandBuffer = _CommandBuffers[_SyncObjects.CurrentFrame];

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}

		VkSemaphore waitSemaphores[] = {_SyncObjects.ImageAvailibleSemaphores[_SyncObjects.CurrentFrame]};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphore[] = {_SyncObjects.RenderFinishedSemaphores[_SyncObjects.CurrentFrame]};

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphore
		};

		if (vkQueueSubmit(_Context.GraphicsQueue, 1, &submitInfo, _SyncObjects.InFlightFences[_SyncObjects.CurrentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		VkSwapchainKHR swapchains[] = { _Swapchain };
		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphore,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &_CurrentImageIndex
		};

		VkResult result = vkQueuePresentKHR(_Context.GraphicsQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			// @Todo: recreate swapchain
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swapchain image!");
		}

		_SyncObjects.CurrentFrame = (_SyncObjects.CurrentFrame + 1) % GxSyncObjects::MAX_FRAMES_IN_FLIGHT;
	}

	void VkRenderer::CreateInstance(HINSTANCE instance, HWND window)
	{
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Engine app",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Archimedes",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_4,
		};

		// @TODO: query required extensions
		std::vector<const char*> extensions = {
			VK_KHR_SURFACE_EXTENSION_NAME
		};

		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = static_cast<u32>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()
		};

#ifdef DEBUG
		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif

		if (vkCreateInstance(&createInfo, nullptr, &_Context.Instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan instance!");
		}
		
		VkWin32SurfaceCreateInfoKHR surfaceInfo{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.hinstance = instance,
			.hwnd = window
		};
			
		VkResult result = vkCreateWin32SurfaceKHR(_Context.Instance, &surfaceInfo, nullptr, &_Context.Surface);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Surfac creation failed");
		}
	}

	bool VkRenderer::IsDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	}

	void VkRenderer::SelectPhysicalDevice() {
		u32 deviceCount = 0;
		vkEnumeratePhysicalDevices(_Context.Instance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		
		for (const auto& device : devices) {
			if (IsDeviceSuitable(device))
			{
				_Context.PhysicalDevice = device;
				break;
			}
		}

		if (_Context.PhysicalDevice == VK_NULL)
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	void VkRenderer::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(_Context.PhysicalDevice);

		float queryPriority = 1.0f;
		VkDeviceQueueCreateInfo  queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = indices.GraphicsFamily.value(),
			.queueCount = 1,
			.pQueuePriorities = &queryPriority
		};
		
		VkPhysicalDeviceFeatures deviceFeatures{};
		std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueCreateInfo,
			.enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &deviceFeatures
		};

		if (vkCreateDevice(_Context.PhysicalDevice, &createInfo, nullptr, &_Context.LogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device");
		}

		vkGetDeviceQueue(_Context.LogicalDevice, indices.GraphicsFamily.value(), 0, &_Context.GraphicsQueue);
	}

	void VkRenderer::CreateSwapchain()
	{
		if (_Context.Surface == VK_NULL)
			return;

		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_Context.PhysicalDevice, _Context.Surface, &capabilities);

		// This still seems dumb
		u32 formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(_Context.PhysicalDevice, _Context.Surface, &formatCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(_Context.PhysicalDevice, _Context.Surface, &formatCount, formats.data());

		VkSurfaceFormatKHR surfaceFormat = formats[0];
		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				surfaceFormat = format;
				break;
			}
		}

		VkExtent2D extent = capabilities.currentExtent;

		u32 imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR  createInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.surface = _Context.Surface,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL
		};
		if (vkCreateSwapchainKHR(_Context.LogicalDevice, &createInfo, nullptr, &_Swapchain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swapchain!");
		}

		_SwapchainFormat = surfaceFormat.format;
		_SwapchainExtent = extent;

		vkGetSwapchainImagesKHR(_Context.LogicalDevice, _Swapchain, &imageCount, nullptr);
		_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(_Context.LogicalDevice, _Swapchain, &imageCount, _SwapchainImages.data());

	}
}