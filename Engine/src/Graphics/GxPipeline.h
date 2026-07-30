#pragma once
#include "archpch.h"

namespace Engine::Graphics
{
	struct GxVertexAttribute
	{
		u32 Location = 0;
		VkFormat Format = VK_FORMAT_R32G32B32_SFLOAT;
		u32 Offset = 0;
	};

	struct GxVertexBinding
	{
		u32 Binding = 0;
		u32 Stride = 0;
		VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		std::vector<GxVertexAttribute> Atrributes;
	};

	struct GxPipelineConfig 
	{
		VkPipelineLayout Layout{ VK_NULL };
		VkRenderPass RenderPass{ VK_NULL };
		u32 Subpass = 0;

		std::filesystem::path& VertexShaderPath;
		std::filesystem::path& FragmentShaderPath;

		std::vector<GxVertexBinding> VertexBindings;

		VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkCullModeFlags CullMode = VK_CULL_MODE_NONE; // @TODO: Change
		VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		bool DepthTestEnabled = false;
		bool DepthWriteEnabled = false;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;

		bool BlendEnable = false;
		VkBlendFactor SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		VkBlendFactor DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		VkBlendFactor SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		VkBlendFactor DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		// No depth test, alpha_blending, no culling -- good default config for 2D
		static GxPipelineConfig Default2D(VkPipelineLayout layout, VkRenderPass renderPass, u32 subpass = 0);

		// Depth test + write on / back-face culling, opaque blending.
		static GxPipelineConfig Default3D(VkPipelineLayout layout, VkRenderPass renderPass, u32 subpass = 0);
	};

	class GxPipeline
	{
	public:
		GxPipeline() = default;
		~GxPipeline();

		void Bind(VkCommandBuffer& commandBuffer);
		void Build(const GxPipelineConfig& config);
	
	private:
		VkPipeline _GraphicsPipeline{ VK_NULL };
	};
} // Namespace Engine::Graphics