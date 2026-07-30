#include "archpch.h"
#include "GxPipeline.h"
#include "VkRenderer.h"

#include <fstream>
#include <stdexcept>

namespace Engine::Graphics
{
	VkContext& GetVkContext();

	GxPipelineConfig GxPipelineConfig::Default2D(VkPipelineLayout layout, VkRenderPass renderPass, u32 subpass)
	{
		GxPipelineConfig config{};
		config.Layout = layout;
		config.RenderPass = renderPass;
		config.Subpass = subpass;

		config.VertexShaderPath = "shaders/sprite.vert.spv";
		config.FragmentShaderPath = "shaders/sprite.vert.spv";

		GxVertexBinding binding{};
		binding.Binding = 0;
		binding.Stride = sizeof(float) * 8;
		binding.InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		binding.Atrributes =
		{
			{0, VK_FORMAT_R32G32_SFLOAT, 0},
			{1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2},
			{2, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 4}
		};
		config.VertexBindings = { binding };

		config.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config.CullMode = VK_CULL_MODE_NONE;
		config.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		config.DepthTestEnable = false;
		config.DepthWriteEnable = false;

		config.BlendEnable = true;
		config.SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		config.DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		config.SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		config.DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		return config;
	}

	GxPipelineConfig GxPipelineConfig::Default3D(VkPipelineLayout layout, VkRenderPass renderPass, u32 subpass)
	{
		GxPipelineConfig config{};
		config.Layout = layout;
		config.RenderPass = renderPass;
		config.Subpass = subpass;

		config.VertexShaderPath = "shaders/mesh.vert.spv";
		config.FragmentShaderPath = "shaders/mesh.vert.spv";

		GxVertexBinding binding{};
		binding.Binding = 0;
		binding.Stride = sizeof(float) * 8;
		binding.InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		binding.Atrributes =
		{
			{0, VK_FORMAT_R32G32_SFLOAT, 0},
			{1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3},
			{2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6}
		};
		config.VertexBindings = { binding };

		config.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config.CullMode = VK_CULL_MODE_BACK_BIT;
		config.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		config.DepthTestEnable = true;
		config.DepthWriteEnable = true;
		config.DepthCompareOp = VK_COMPARE_OP_LESS;

		config.BlendEnable = false;
		
		return config;
	}

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

	void GxPipeline::Bind(VkCommandBuffer& commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _GraphicsPipeline);
	}

	void GxPipeline::Build(const GxPipelineConfig& config)
	{
		VkContext& ctx = GetVkContext();

		auto vertCode = ReadFile(config.VertexShaderPath);
		auto fragCode = ReadFile(config.FragmentShaderPath);

		VkShaderModule vertModule = CreateShaderModule(ctx.LogicalDevice, vertCode);
		VkShaderModule fragModule = CreateShaderModule(ctx.LogicalDevice, fragCode);
		
		VkPipelineShaderStageCreateInfo vertStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertModule,
			.pName = "main",
		};

		VkPipelineShaderStageCreateInfo fragStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragModule, 
			.pName = "main",
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };
		
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		for (const auto& binding : config.VertexBindings)
		{
			VkVertexInputBindingDescription bindingDesc{
				.binding = binding.Binding,
				.stride = binding.Stride,
				.inputRate = binding.InputRate
			};
			bindingDescriptions.push_back(bindingDesc); 

			for (const auto& attribute : binding.Atrributes)
			{
				VkVertexInputAttributeDescription  attributeDesc{};
				attributeDesc.binding = binding.Binding;
				attributeDesc.location = attribute.Location;
				attributeDesc.format = attribute.Format;
				attributeDesc.offset = attribute.Offset;
				attributeDescriptions.push_back(attributeDesc);
			}
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = static_cast<u32>(bindingDescriptions.size()),
			.pVertexBindingDescriptions = bindingDescriptions.data(),
			.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data()
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = config.Topology,
			.primitiveRestartEnable = VK_FALSE
		};

		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1
		};

		VkPipelineRasterizationStateCreateInfo rasterizer{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		};
		rasterizer.depthClampEnable = VK_FALSE;
		// rasterizer.polygonMode = config.
	}

} // namespace Engine::Graphics