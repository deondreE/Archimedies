#pragma once
#include "archpch.h"

namespace Engine::Graphics
{
	enum class BufferUsage
	{
		Vertex,
		Index,
		Uniform,
		Storage
	};

	struct GxBuffer 
	{
		VkBuffer Handle{ VK_NULL };
		VkDeviceMemory Memory{ VK_NULL };
		VkDeviceSize Size{ 0 };

		void Create(VkDeviceSize size, BufferUsage usage);
		void Destroy();
		void Map(void** data);
		void Unmap();
	}; 
} // Namespace Engine::Graphics