#pragma once

#include "core.hpp"	

namespace arc {

	class Shader
	{
		std::vector<vk::ShaderEXT> make_shader_object(vk::Device logicalDevice,
			const char* vertexFileName, const char* fragementFileName,
			vk::DispatchLoaderDynamic& dl, std::deque<std::function<void(vk::Device) >> &deviceDeletionQueue);
	};
}

