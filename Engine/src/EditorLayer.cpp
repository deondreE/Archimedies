#include "archpch.h"
#include "EditorLayer.h"
#include "Renderer.h" 
#include "Input.h"
#include "Texture.h"
#include "imgui_internal.h"
#include "AudioEngine.h"
#include <deque>

namespace Engine {

	// @Info: Should the Editor UI Be Toggable ?
	// Just focus on the Viewport for the most part, panels are transparent on top of the viewport.

	void EditorLayer::OnMenuBarRender() {
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Hierarchy", nullptr, &_ShowHierarchy);
			ImGui::MenuItem("Inspector", nullptr, &_ShowInspector);
			ImGui::MenuItem("Lighting", nullptr, &_ShowLighting);
			ImGui::MenuItem("Stats", nullptr, &_ShowStats);
			ImGui::MenuItem("Content Browser", nullptr, &_ShowContentBrowser);
			ImGui::EndMenu();
		}
	}


	void EditorLayer::OnImGuiRender() {
		auto& entities = _Scene->GetEntities();
	
		if (_ShowContentBrowser) {
			_ContentBrowser.OnImGuiRender(_ShowContentBrowser);
		}

		// Entity Panel List
		if (_ShowHierarchy) {
			ImGui::Begin("Scene");
			for (int i = 0; i < (int)entities.size(); ++i) {
				bool isSelected = (_SelectedIndex == i);
				if (ImGui::Selectable(entities[i].Name.c_str(), isSelected)) {
					_SelectedIndex = i;
				}
			}
			ImGui::End();

			ImGui::Begin("Audio");
				float masterVol = Audio::AudioEngine::GetBusVolume(Audio::BusType::Master);
				ImGui::DragFloat("Master Volume", &masterVol, 0.1f, 0.0, 1.0f);
				Audio::AudioEngine::SetBusVolume(Audio::BusType::Master, masterVol);

				float sfxVol = Audio::AudioEngine::GetBusVolume(Audio::BusType::SFX);
				ImGui::DragFloat("SFX Volume", &sfxVol, 0.1f, 0.0, 1.0f);
				Audio::AudioEngine::SetBusVolume(Audio::BusType::SFX, sfxVol);

				float musicVolume = Audio::AudioEngine::GetBusVolume(Audio::BusType::Music);
				ImGui::DragFloat("Music Volume", &musicVolume, 0.1f, 0.0, 1.0f);
				Audio::AudioEngine::SetBusVolume(Audio::BusType::Music, musicVolume);

			ImGui::End();
		}

		if (_ShowInspector) {
			// Inspector panel for the selected entity
			ImGui::Begin("Inspector");
			if (_SelectedIndex >= 0 && _SelectedIndex < (int)entities.size()) {
				Entity& e = entities[_SelectedIndex];

				ImGui::Text("Name: %s", e.Name.c_str());
				ImGui::Separator();

				ImGui::DragFloat3("Position", &e.Position.x, 0.1f, 0, 100.0f);
				ImGui::DragFloat3("Scale", &e.Scale.x, 0.1f, 0, 100.0f);
				ImGui::DragFloat3("Rotation", &e.Rotation.x, 0.1f); // @Todo: Make this render as Deg 0->360

				ImGui::Separator();

				ImGui::Text("Texture");
				ImGui::SameLine();

				std::string textureLabel = "None";
				if (e.Material && e.Material->GetTexture()) textureLabel = "Assinged";

				ImGui::Button(textureLabel.c_str(), ImVec2(120, 0));

				// Render the specific component the way it wants.
				for (auto& comp : e.Components)
				{
					comp->OnImGuiRender();
				}

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_TEXTURE")) {
						std::string path((const char*)payload->Data, payload->DataSize - 1); // -1: drop the null terminator we included in DataSize

						auto newTexture = Engine::Texture2D::Create(_Device, path);
						if (newTexture && e.Material) {
							e.Material->SetTexture(newTexture);
						}
						ImGui::EndDragDropTarget();
					}
				}

				ImGui::Separator();
				ImGui::Text("Mesh: %s", e.Mesh ? "Assigned" : "None");
				ImGui::Text("Material: %s", e.Material ? "Assigned" : "None");
			}
			else {
				ImGui::TextDisabled("No Entity Selected");
			}

			ImGui::End();
		}
		
		if (_ShowLighting) {
			ImGui::Begin("Lighting");
			auto& light = Renderer::GetLight();
			ImGui::DragFloat3("Direction", &light.Direction.x, 0.01f, -100.0f, 100.0f);
			ImGui::DragFloat("Intensity", &light.Itensity, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("Ambient", &light.AmbientStrength, 0.01f, 0.0f, 1.0f);
			ImGui::ColorEdit3("Color", light.Color);
			ImGui::End();
		}

		if (_ShowStats) {
			ImGui::Begin("Stats");
			ImGui::Text("Frame Time: %.3f ms", ImGui::GetIO().DeltaTime * 1000.0f);
			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::Text("Entities: %zu", entities.size());
			ImGui::End();
		}

		ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");
		ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspaceID);

		if (centralNode) {
			ImVec2 pos = centralNode->Pos;
			ImVec2 size = centralNode->Size;
		
			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);

			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking |
				ImGuiWindowFlags_NoNavFocus; 

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("ViewportDropTarget", nullptr, flags);
			ImGui::PopStyleVar();

			ImGui::InvisibleButton("##viewportdrop", ImGui::GetContentRegionAvail());

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload =
					ImGui::AcceptDragDropPayload("CONTENT_BROWSER_MESH")) {
					std::string path((const char*)payload->Data, payload->DataSize - 1);
					LOG_INFO("Dropped mesh into scene: %s", path.c_str());

					auto& t = _Scene->CreateEntity("");
					t.Name = "Model_" + t.GetID();
					auto material = Material::GetDefault(_Device);
					auto mesh = Mesh::LoadFromFile(_Device, path);
					t.Position = { 0, 0, 0 };
					t.Mesh = mesh;
					t.Material = material;
				}
				if (const ImGuiPayload* payload =
					ImGui::AcceptDragDropPayload("CONTENT_BROWSER_TEXTURE")) {
					std::string path((const char*)payload->Data, payload->DataSize - 1);
					LOG_INFO("Dropped texture into scene: %s", path.c_str());

					// @Todo: Textures do not render.
					auto& t = _Scene->CreateEntity("");
					t.Name = "Texture_" + t.GetID();
					std::vector<Engine::Vertex> quad_verts = {
						{{-0.5f,  0.5f, 0.0f}, {1, 1, 1, 1}, {0.0f, 0.0f}, {0, 0, -1}}, // top-left
						{{ 0.5f,  0.5f, 0.0f}, {1, 1, 1, 1}, {1.0f, 0.0f}, {0, 0, -1}}, // top-right
						{{ 0.5f, -0.5f, 0.0f}, {1, 1, 1, 1}, {1.0f, 1.0f}, {0, 0, -1}}, // bottom-right
						{{-0.5f, -0.5f, 0.0f}, {1, 1, 1, 1}, {0.0f, 1.0f}, {0, 0, -1}}, // bottom-left
					};
					std::vector<uint32_t> indices = {
						0, 1, 2,
						0, 2, 3
					};
					auto mesh = Engine::Mesh::Create(_Device, quad_verts, indices);
					auto texture = Engine::Texture2D::Create(_Device, path);
					std::cout << path;
					auto shader = Engine::Shader::Create(_Device, Material::GetDefaultShaderPath());
					auto mat = std::make_shared<Material>(shader, texture);
					t.Mesh = mesh;
					t.Material = mat;
					t.Position = { 0, 0, 0 };
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::End();
		}
	}
}