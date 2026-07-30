#pragma once
#include "archpch.h"

namespace Engine::Graphics 
{
	struct VkContext 
	{
		VkInstance Instance = VK_NULL;
		VkPhysicalDevice PhysicalDevice = VK_NULL;
		VkDevice LogicalDevice = VK_NULL;
		VkQueue	GraphicsQueue = VK_NULL;
		VkSurfaceKHR Surface = VK_NULL;
	};

	struct GxSyncObjects
	{
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		VkSemaphore ImageAvailibleSemaphores[MAX_FRAMES_IN_FLIGHT];
		VkSemaphore RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
		VkFence InFlightFences[MAX_FRAMES_IN_FLIGHT];

		u32 CurrentFrame = 0;
	};

	class VkRenderer 
	{
	public:
		VkRenderer() = default;
		~VkRenderer();

		void Init(void* windowHandle);
		void Shutdown();

		void BeginFrame();
		void EndFrame();
	private:
		void CreateInstance();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapchain();

		bool IsDeviceSuitable(VkPhysicalDevice device);
	private:
		VkContext _Context;
		VkCommandPool _CommandPool{ VK_NULL };
		std::vector<VkCommandBuffer> _CommandBuffers;
	
	};
} // Namespace Engine::Graphics