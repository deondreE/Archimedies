#include "archpch.h"
#include "GxPipeline.h"
#include "VkRenderer.h"

#include <fstream>
#include <stdexcept>

namespace Engine::Graphics
{
	VkContext& GetVkContext();

	namespace 
	{
		std::vector<char> ReadFile(const std::string& path)
		{
			std::ifstream file(path, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				throw std::runtime_error("Failed to open shader file: " + path);
			}

			size_t fileSize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		// @Todo: Port Shader.h / ShaderLibrary.h to work with Vulkan.
		VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const u32*>(code.data());

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create shader module!");
			}

			return shaderModule;
		}
	} // namespace 

	GxPipeline::~GxPipeline()
	{
		if (_GraphicsPipeline != VK_NULL)
		{
			VkContext& ctx = GetVkContext();
			vkDestroyPipeline(ctx.LogicalDevice, _GraphicsPipeline, nullptr);
			_GraphicsPipeline = VK_NULL;
		}
	}
} // namespace Engine::Graphics