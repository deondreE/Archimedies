#pragma once

#include "archpch.h"
#include "Shader.h"

namespace Engine {
	class ShaderLibrary {
	public:
		// Loads (or returns cached) shader by name, compiling from path if not already loaded.
		std::shared_ptr<Shader> Load(ID3D11Device* device, const std::string& name, const std::wstring& path);
		
		// Retrieve an already-loaded shader by name. Returns nullptr if not found (logs a warning).
		std::shared_ptr<Shader> Get(const std::string& name);

		bool Exists(const std::string& name) const;

		void CheckForChanges(ID3D11Device* device);
	private:
		struct Entry {
			std::shared_ptr<Shader> ShaderPtr;
			FILETIME LastWriteTime;
		};

		static bool GetFileWriteTime(const std::wstring& path, FILETIME& outTime);

		std::unordered_map<std::string, Entry> _Shaders;
	};
}
