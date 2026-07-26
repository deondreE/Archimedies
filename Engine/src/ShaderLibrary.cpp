#include "archpch.h"
#include "ShaderLibrary.h"

namespace Engine {

	std::shared_ptr<Shader> ShaderLibrary::Load(ID3D11Device* device, const std::string& name, const std::wstring& path) {
		auto it = _Shaders.find(name);
		if (it != _Shaders.end()) {
			LOG_TRACE("ShaderLibrary: '%s' already loaded, returning cached instance", name.c_str());
			return it->second.ShaderPtr;
		}

		auto shader = Shader::Create(device, path);
		if (!shader) {
			LOG_ERROR("ShaderLibrary: failed to load '%s' from path", name.c_str());
			return nullptr;
		}

		Entry entry;
		entry.ShaderPtr = shader;
		if (!GetFileWriteTime(path, entry.LastWriteTime)) {
			LOG_WARN("ShaderLibrary: couldn't stat '%s' for hot-reload tracking", name.c_str());
			entry.LastWriteTime = {}; // hot-reload just won't trigger for this one; shader still works
		}

		_Shaders[name] = entry;
		LOG_INFO("ShaderLibrary: loaded '%s'", name.c_str());
		return shader;
	}

	std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) {
		auto it = _Shaders.find(name);
		if (it == _Shaders.end()) {
			LOG_WARN("ShaderLibrary: shader '%s' not found", name.c_str());
			return nullptr;
		}
		return it->second.ShaderPtr;
	}

	bool ShaderLibrary::Exists(const std::string& name) const {
		return _Shaders.find(name) != _Shaders.end(); 
	}

	void ShaderLibrary::CheckForChanges(ID3D11Device* device) {
		for (auto& [name, entry] : _Shaders) {
			FILETIME currentWriteTime;
			if (!GetFileWriteTime(entry.ShaderPtr->GetPath(), currentWriteTime)) {
				continue; // file temporarily inaccessable
			}

			if (CompareFileTime(&currentWriteTime, &entry.LastWriteTime) != 0) {
				LOG_INFO("ShaderLibrary: detected change in '%s' reloading", name.c_str());
				entry.LastWriteTime = currentWriteTime;

				entry.ShaderPtr->Reload(device);
			}
		}
	}

	bool ShaderLibrary::GetFileWriteTime(const std::wstring& path, FILETIME& outTime) {
		WIN32_FILE_ATTRIBUTE_DATA data; 
		if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
			return false;
		}
		outTime = data.ftLastWriteTime;
		return true;
	}
}