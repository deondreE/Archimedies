#include "VkRenderer.h"
#include "archpch.h"

namespace Engine::Graphics {

	void VkRenderer::Init(void* windowHandle) {
		_CommandBuffers.resize(GxSyncObjects::MAX_FRAMES_IN_FLIGHT);
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
	}

	

}