VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/core/vendor/GLFW/include"
IncludeDir["Glad"] = "%{wks.location}/core/vendor/Glad/include"
IncludeDir["VulkanSDK"] = "C:/VulkanSDK/1.3.290.0/Include"

LibraryDir = {}

LibraryDir["VulkanSDK"] = "C:/VulkanSDK/1.3.290.0/Lib"

Library = {}
Library["Vulkan"] = "C:/VulkanSDK/1.3.290.0/vulkan-1.lib"