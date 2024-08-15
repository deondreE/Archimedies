#include "Shader.hpp"

#include "../backend/File.hpp"

namespace arc {
	std::vector<vk::ShaderEXT> Shader::make_shader_object(vk::Device logicalDevice, const char* vertexFileName, const char* fragementFileName, vk::DispatchLoaderDynamic& dl, std::deque<std::function<void(vk::Device)>>& deviceDeletionQueue)
	{
		vk::ShaderCreateFlagsEXT flags = vk::ShaderCreateFlagBitsEXT::eLinkStage;
		vk::ShaderStageFlags nextStage = vk::ShaderStageFlagBits::eFragment;

		std::vector <char> vertexSrc = read_file(vertexFileName);
		vk::ShaderCodeTypeEXT codeType = vk::ShaderCodeTypeEXT::eSpirv;
		const char* pName = "main";

		vk::ShaderCreateInfoEXT vertexInfo = {};
		vertexInfo.setFlags(flags);
		vertexInfo.setStage(vk::ShaderStageFlagBits::eVertex);
		vertexInfo.setNextStage(nextStage);
		vertexInfo.setCodeType(codeType);
		vertexInfo.setCodeSize(vertexSrc.size());
		vertexInfo.setPCode(vertexSrc.data());
		vertexInfo.setPName(pName);

		std::vector<char> fragmentSrc = read_file(fragementFileName);
		vk::ShaderCreateInfoEXT fragmentInfo{};
		fragmentInfo.setFlags(flags);
		fragmentInfo.setStage(vk::ShaderStageFlagBits::eFragment);
		fragmentInfo.setCodeType(codeType);
		fragmentInfo.setCodeSize(fragmentSrc.size());
		fragmentInfo.setPCode(fragmentSrc.data());
		fragmentInfo.setPName(pName);

		std::vector<vk::ShaderCreateInfoEXT> shaderInfo;
		shaderInfo.push_back(vertexInfo);
		shaderInfo.push_back(fragmentInfo);

		auto result = logicalDevice.createShadersEXT(shaderInfo, nullptr, dl);
		std::vector<vk::ShaderEXT> shaders;

		if (result.result == vk::Result::eSuccess) {
			std::cout << "Sucecssfully Made Shaders" << '\n';
			shaders = result.value;
			VkShaderEXT vertexShader = shaders[0];
			deviceDeletionQueue.push_back([vertexShader, dl](vk::Device device) {
				device.destroyShaderEXT(vertexShader, nullptr, dl);
				});
			VkShaderEXT fragmentShader = shaders[1];
			deviceDeletionQueue.push_back([fragmentShader, dl](vk::Device device) {
				device.destroyShaderEXT(fragmentShader, nullptr, dl);
				});
		}
		else {
			std::runtime_error("Shader creation failed");
		}
		return shaders;
	}
}
