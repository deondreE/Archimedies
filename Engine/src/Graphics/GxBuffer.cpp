#include "archpch.h"
#include "GxBuffer.h"
#include "VkRenderer.h"

#include <stdexcept>

namespace Engine::Graphics
{
	VkContext& GetVkContext();

	namespace 
	{
		VkBufferUsageFlags ToVkUsage(BufferUsage usage) {
			switch (usage) {
			case BufferUsage::Vertex: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case BufferUsage::Index: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case BufferUsage::Uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			case BufferUsage::Storage: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; 
			}
			return 0;
		}

		u32 FindMemoryType(VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (u32 i = 0; i < memProperties.memoryTypeCount; ++i)
			{
				bool typeSupported = (typeFilter & (1 << i)) != 0;
				bool propertiesSupported =
					(memProperties.memoryTypes[i].propertyFlags & properties) == properties;

				if (typeSupported && propertiesSupported)
				{
					return i;
				}
			}
			throw std::runtime_error("Failed to find suitable memory type!");
		}
	}

	void GxBuffer::Create(VkDeviceSize size, BufferUsage usage)
	{
		VkContext& ctx = GetVkContext();

		Size = size;

		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = ToVkUsage(usage),
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		if (vkCreateBuffer(ctx.LogicalDevice, &bufferInfo, nullptr, &Handle) != VK_SUCCESS)
		{
			LOG_ERROR("Failed to create vkBuffer");
			throw std::runtime_error("create buffer failed!");
		}
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(ctx.LogicalDevice, Handle, &memRequirements);

		// Host Visible/Coherent
		VkMemoryPropertyFlags memoryProperties =
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = FindMemoryType(
				ctx.PhysicalDevice, memRequirements.memoryTypeBits, memoryProperties)
		};

		if (vkAllocateMemory(ctx.LogicalDevice, &allocInfo, nullptr, &Memory) != VK_SUCCESS)
		{
			// Am i going to need a callback here ?
			vkDestroyBuffer(ctx.LogicalDevice, Handle, nullptr);
			Handle = VK_NULL;
			LOG_ERROR("Failed to allocate buffer memory.");
			throw std::runtime_error("Failed to allocate buffer memory.");
		}

		vkBindBufferMemory(ctx.LogicalDevice, Handle, Memory, 0);
 	}

	void GxBuffer::Destroy()
	{
		VkContext& ctx = GetVkContext();

		if (Handle != VK_NULL)
		{
			vkDestroyBuffer(ctx.LogicalDevice, Handle, nullptr); 
			Handle = VK_NULL ;
		}

		if (Memory != VK_NULL)
		{
			vkFreeMemory(ctx.LogicalDevice, Memory, nullptr);
			Memory = VK_NULL;
		}

		Size = 0;
	}

	void GxBuffer::Map(void** data)
	{
		VkContext& ctx = GetVkContext();
		VkMemoryMapInfo mmInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
			.pNext = 0,
			.flags = 0,
			.memory = Memory,
			.offset = 0,
			.size = Size
		};
		// @Todo: can't this fail ??
		vkMapMemory2(ctx.LogicalDevice, &mmInfo, data);
	}

	void GxBuffer::Unmap()
	{
		VkContext& ctx = GetVkContext();
		VkMemoryUnmapInfo umInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
			.pNext = 0,
			.flags = 0,
			.memory = Memory
		};
		vkUnmapMemory2(ctx.LogicalDevice, &umInfo);
	}
}