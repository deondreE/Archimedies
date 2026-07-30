#pragma once
#include "archpch.h"

namespace Engine::Graphics
{
	struct GxPipelineConfig 
	{
		VkPipelineLayout Layout{ VK_NULL };
		VkRenderPass RenderPass{ VK_NULL };
		u32 Subpass = 0;
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