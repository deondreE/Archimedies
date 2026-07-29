#pragma once
#include "archpch.h"
#include "Texture.h"
#include <filesystem> 

namespace fs = std::filesystem;

namespace Engine {
	class ContentBrowserPanel {
	public:
		ContentBrowserPanel(ID3D11Device* device, const fs::path& assetRoot);

		void OnImGuiRender(bool& open);
	private:
		void DrawBreadcrumbs();
		void DrawGrid();

		std::shared_ptr<Texture2D> GetOrLoadThumbnail(const fs::path& path);
		std::shared_ptr<Texture2D> _MeshIconTexture;
		std::shared_ptr<Texture2D> _FileIconTexture;
		std::shared_ptr<Texture2D> _DirIconTexture;
		std::shared_ptr<Texture2D> _SoundFileIconTexture;

		ID3D11Device* _Device;
		fs::path _AssetRoot;
		fs::path _CurrentDir;

		float _ThumbnailSize = 80.0f;
		float _Padding = 12.0f;
		
		std::unordered_map<std::string, std::shared_ptr<Texture2D>> _ThumbnailCache;
	};
}