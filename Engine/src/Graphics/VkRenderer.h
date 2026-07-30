#pragma once
#include "archpch.h"

namespace Engine::Graphics 
{
	struct VkContext 
	{
		VkInstance Instance{ VK_NULL };
		VkPhysicalDevice PhysicalDevice{ VK_NULL };
		VkDevice LogicalDevice{ VK_NULL };
		VkQueue	GraphicsQueue{ VK_NULL };
		VkSurfaceKHR Surface{ VK_NULL };
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

		// Needs HIsntance and HWND
		void Init(void* windowHandle, void* instance);
		void Shutdown();

		void BeginFrame();
		void EndFrame();
	private:
		void CreateInstance(HINSTANCE instance, HWND window);
		void SelectPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapchain();

		bool IsDeviceSuitable(VkPhysicalDevice device);
	private:
		VkContext _Context;
		VkCommandPool _CommandPool{ VK_NULL };
		std::vector<VkCommandBuffer> _CommandBuffers;
		GxSyncObjects _SyncObjects;
		VkSwapchainKHR _Swapchain{ VK_NULL };
		std::vector<VkImage> _SwapchainImages;
		VkExtent2D _SwapchainExtent;
		VkFormat _SwapchainFormat;
		u32 _CurrentImageIndex = 0;
	};
} // Namespace Engine::Graphics