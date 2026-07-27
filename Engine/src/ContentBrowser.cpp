#include "archpch.h"
#include "ContentBrowser.h"
#include "AssetLoaders/AsespriteLoader.h"

namespace Engine {

	static bool IsImageFile(const fs::path& path) {
		std::string ext = path.extension().string();
		return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" 
			|| ext == ".tga" || ext == ".ase" || ext == ".aseprite";
	}

	ContentBrowserPanel::ContentBrowserPanel(ID3D11Device* device, const fs::path& assetRoot)
		: _AssetRoot(assetRoot), _CurrentDir(assetRoot), _Device(device) {
		std::error_code ec;
		fs::path resolved = fs::absolute(assetRoot, ec);

		

		if (ec) {
			LOG_ERROR("ContentBrowserPanel: failed to resolve asset root '%s': %s",
			assetRoot.string().c_str(), ec.message().c_str());
			resolved = assetRoot;
		}
		_AssetRoot = resolved;
		_CurrentDir = resolved;
	}

	std::shared_ptr<Texture2D> ContentBrowserPanel::GetOrLoadThumbnail(const fs::path& path) {
		assert(_Device);
		std::string key = path.string();

		auto it = _ThumbnailCache.find(key);
		if (it != _ThumbnailCache.end()) {
			return it->second; // may be nullptr;
		}

		std::shared_ptr<Texture2D> texture;

		std::string ext = path.extension().string();
		std::transform(
			ext.begin(),
			ext.end(),
			ext.begin(),
			[](unsigned char c) { return (char)std::tolower(c);});
		if (ext == ".ase" || ext == ".aseprite") {
			auto sprite = Loaders::AsepriteLoader::Load(key);

			if (sprite &&
				!sprite->Frames.empty() &&
				!sprite->Frames[0].CompositePixels.empty())
			{
				texture = Texture2D::CreateFromRGBA(
					_Device,
					sprite->Width,
					sprite->Height,
					sprite->Frames[0].CompositePixels.data());
			}
		}
		else {
			texture = Texture2D::Create(
				_Device,
				key);
		}

		if (!texture)
		{
			LOG_WARN(
				"ContentBrowser: failed thumbnail load %s",
				key.c_str());
		}

		_ThumbnailCache[key] = texture;
		return texture;
	}

	void ContentBrowserPanel::OnImGuiRender(bool& open) {
		ImGui::Begin("Content Browser", &open);

		DrawBreadcrumbs();
		ImGui::Separator();
		DrawGrid();

		ImGui::End();
	}

	void ContentBrowserPanel::DrawBreadcrumbs() {
		// Build the path segments between AssetRoot and CurrentDir so each can be clicked
		// to jump straight back to that level (e.g. Assets > Models > Props)
		fs::path relative = fs::relative(_CurrentDir, _AssetRoot.parent_path());
		
		std::vector<fs::path> segments;
		fs::path accum = _AssetRoot.parent_path();
		for (const auto& part : relative) {
			accum /= part;
			segments.push_back(accum);
		}

		for (size_t i = 0; i < segments.size(); i++) {
			if (i > 0) {
				ImGui::SameLine();
				ImGui::TextDisabled(">");
				ImGui::SameLine();
			}
			std::string label = segments[i].filename().string();
			if (ImGui::Button(label.c_str())) {
				_CurrentDir = segments[i];
			}
		}
	}

	void ContentBrowserPanel::DrawGrid() {
		float cellSize = _ThumbnailSize + _Padding;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = std::max(1, (int)(panelWidth / cellSize));

		ImGui::Columns(columnCount, nullptr, false);

		// "UP" entry unless we're at the root
		if (_CurrentDir != _AssetRoot) {
			if (ImGui::Button("../", ImVec2(_ThumbnailSize, _ThumbnailSize))) {
				_CurrentDir = _CurrentDir.parent_path();
			}
			ImGui::TextWrapped("..");
			ImGui::NextColumn();
		}

		try {
			for (auto& entry : fs::directory_iterator(_CurrentDir)) {
				const auto& path = entry.path();
				std::string filename = path.filename().string();

				ImGui::PushID(filename.c_str());

				if (entry.is_directory()) {
					if (ImGui::Button("[Dir]", ImVec2(_ThumbnailSize, _ThumbnailSize))) {
						// single check just selects the button visually.
					}
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						_CurrentDir = path;
					}
				}
				else if (IsImageFile(path)) {
					auto thumbnail = GetOrLoadThumbnail(path);
					if (thumbnail) {
						ImGui::ImageButton(filename.c_str(), (ImTextureID)thumbnail->GetSRV(),
							ImVec2(_ThumbnailSize, _ThumbnailSize));
					}
					else {
						ImGui::Button("[Err]", ImVec2(_ThumbnailSize, _ThumbnailSize)); // load failed, fall back
					}

					if (ImGui::BeginDragDropSource()) {
						std::string fullPath = path.string();
						ImGui::SetDragDropPayload("CONTENT_BROWSER_TEXTURE", fullPath.c_str(), fullPath.size() + 1);
						if (thumbnail) {
							ImGui::Image((ImTextureID)thumbnail->GetSRV(), ImVec2(32, 32));
							ImGui::SameLine();
						}
						ImGui::Text("%s", filename.c_str());
						ImGui::EndDragDropSource();
					}
				}
				else {
					std::string ext = path.extension().string();
					const char* icon = "[File]";
					const char* payloadType = "CONTENT_BROWSER_ITEM";
					if (ext == "hlsl" || ext == "glsl" || ext == "metal") { icon = "[Shdr]";  payloadType = "CONTENT_BROWSER_SHADER"; }
					else if (ext == ".obj" || ext == ".fbx" || ext == ".glb") { icon = "[Mesh]"; payloadType = "CONTENT_BROWSER_MESH"; }

					ImGui::Button(icon, ImVec2(_ThumbnailSize, _ThumbnailSize));

					if (ImGui::BeginDragDropSource()) {
						std::string fullPath = path.string();
						ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", fullPath.c_str(), fullPath.size() + 1);
						ImGui::Text("%s", filename.c_str());
						ImGui::EndDragDropSource();
					}
				}

				ImGui::TextWrapped("%s", filename.c_str());
				ImGui::PopID();
				ImGui::NextColumn();
			}
		}
		catch (const fs::filesystem_error& ex) {
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Error reading directory: %s", ex.what());
		}

		ImGui::Columns(1);
	}
}