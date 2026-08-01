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
				float sfxVol = Audio::AudioEngine::GetBusVolume(Audio::BusType::SFX);
				float musicVolume = Audio::AudioEngine::GetBusVolume(Audio::BusType::Music);

				if (Knob("Master", &masterVol, 0.0f, 1.0f))
					Audio::AudioEngine::SetBusVolume(Audio::BusType::Master, masterVol);

				if (Knob("SFX", &sfxVol, 0.0f, 1.0f))
					Audio::AudioEngine::SetBusVolume(Audio::BusType::SFX, sfxVol);

				if (Knob("Music", &musicVolume, 0.0f, 1.0f))
					Audio::AudioEngine::SetBusVolume(Audio::BusType::Music, musicVolume);
			ImGui::End();
		}

		if (_ShowInspector) {
			// Inspector panel for the selected entity
			ImGui::Begin("Inspector");
			// Scene Camera
			ImGui::Begin("Camera");
			Math::Vec3 pos = _Scene->PrimaryCamera.GetPosition();
			if (DragFloat3Colored("Position", &pos.x, 0.1f, -1000.0f, 1000.0f))
				_Scene->PrimaryCamera.SetPosition(pos);

			float nPlane = _Scene->PrimaryCamera.GetNearPlane();
			float fPlane = _Scene->PrimaryCamera.GetFarPlane();

			if (ImGui::DragFloat("Near Plane", &nPlane, 0.01f, 0.001f, fPlane - 0.01f))
				_Scene->PrimaryCamera.SetNearPlane(nPlane);

			if (ImGui::DragFloat("Far Plane", &fPlane, 1.0f, nPlane + 0.01f, 10000.0f))
				_Scene->PrimaryCamera.SetFarPlane(fPlane);

			float pitch = _Scene->PrimaryCamera.GetPitch();
			float yaw = _Scene->PrimaryCamera.GetYaw();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 4.0f));
			ImGui::PushItemWidth(100.0f);

			if (ImGui::DragFloat("Yaw", &yaw, 0.1f, -180.0f, 180.0f))
				_Scene->PrimaryCamera.SetYaw(yaw);

			ImGui::SameLine();
			
			if (ImGui::DragFloat("Pitch", &pitch, 0.1f, 0, 100.0f))
				_Scene->PrimaryCamera.SetPitch(pitch);
		
			ImGui::PopItemWidth();
			ImGui::PopStyleVar();

			ImGui::End();
			if (_SelectedIndex >= 0 && _SelectedIndex < (int)entities.size()) {
				Entity& e = entities[_SelectedIndex];

				ImGui::Text("Name: %s", e.Name.c_str());
				ImGui::Separator();

				DragFloat3Colored("Position", &e.Position.x, 0.1f, 0, 100.0f);
				DragFloat3Colored("Scale", &e.Scale.x, 0.1f, 0, 100.0f);
				DragRotationDeg3("Rotation", &e.Rotation.x);

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
			DragFloat3Colored("Direction", &light.Direction.x, 0.01f, -100.0f, 100.0f);
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

			ImGui::Dummy(ImGui::GetContentRegionAvail());

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

		
			if (ImGui::IsKeyPressed(ImGuiKey_W)) _GizmoMode = Engine::GizmoMode::Translate;
			if (ImGui::IsKeyPressed(ImGuiKey_E)) _GizmoMode = Engine::GizmoMode::Rotate;
			if (ImGui::IsKeyPressed(ImGuiKey_R)) _GizmoMode = Engine::GizmoMode::Scale;

			bool gizmoConsumedClick = false;

			auto worldToScreen = [&](const Math::Vec3& world, ImVec2& outScreen) -> bool {
				const Math::Mat4& viewProj = _Scene->PrimaryCamera.GetViewProjection();
				Math::Vec4 clip = viewProj.Transform(Math::Vec4(world, 1.0f));
				if (clip.w <= 0.0001f) return false;
				float ndcX = clip.x / clip.w;
				float ndcY = clip.y / clip.w;
				outScreen.x = pos.x + (ndcX * 0.5f + 0.5f) * size.x;
				outScreen.y = pos.y + (1.0f - (ndcY * 0.5f + 0.5f)) * size.y;
				return true;
			};

			if (_SelectedIndex >= 0 && _SelectedIndex < static_cast<int>(entities.size())) {
				Entity& e = entities[_SelectedIndex];

				_Gizmo.Manipulate(worldToScreen, e.Position, e.Rotation, e.Scale, _GizmoMode); 
				gizmoConsumedClick = _Gizmo.IsUsing();
			}

			if (!gizmoConsumedClick && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				ImVec2 mouse = ImGui::GetIO().MousePos;
				int closest = -1;
				float bestDistSq = 20.0f * 20.0f; // pixel radius threshold, squared

				for (int i = 0; i < (int)entities.size(); i++) {
					ImVec2 screenPos;
					if (!worldToScreen(entities[i].Position, screenPos)) continue;

					float dx = screenPos.x - mouse.x;
					float dy = screenPos.y - mouse.y;
					float distSq = dx * dx + dy * dy;

					if (distSq < bestDistSq) { bestDistSq = distSq; closest = i; }
				}

				_SelectedIndex = closest;
			}

			ImGui::End();
		}
	}

	bool EditorLayer::DragFloat3Colored(const char* label, float* v, float v_speed,
		float v_min, float v_max, const char* format)
	{
		bool changed = false;
		ImGui::PushID(label);
		ImGui::BeginGroup();

		static const char* axis[3] = { "X", "Y", "Z" };
		static const ImVec4 axisColor[3] = {
			ImVec4(0.90f, 0.20f, 0.20f, 1.0f),
			ImVec4(0.20f, 0.80f, 0.20f, 1.0f),
			ImVec4(0.20f, 0.45f, 0.95f, 1.0f)
		};

		float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		float labelW = ImGui::CalcTextSize("X").x + spacing;
		float boxWitdth = (ImGui::CalcItemWidth() - spacing * 2 - labelW * 3) / 3.0f;

		for (int i = 0; i < 3; i++)
		{
			if (i > 0) ImGui::SameLine(0, spacing);
			ImGui::TextColored(axisColor[i], "%s", axis[i]);
			ImGui::SameLine(0, spacing * 0.5f);
			ImGui::PushID(i);
			ImGui::SetNextItemWidth(boxWitdth);
			if (ImGui::DragFloat("##v", &v[i], v_speed, v_min, v_max, format))
				changed = true;
			ImGui::PopID();
		}

		ImGui::SameLine(0, spacing);
		ImGui::TextUnformatted(label);

		ImGui::EndGroup();
		ImGui::PopID();
		return changed;
	}

	bool EditorLayer::DragRotationDeg3(const char* label, float* rotRadians, float v_speed)
	{
		float deg[3] = {
			rotRadians[0] * (180.0f / Math::PI),
			rotRadians[1] * (180.0f / Math::PI),
			rotRadians[2] * (180.0f / Math::PI)
		};

		bool changed = DragFloat3Colored(label, deg, v_speed, 0.0f, 360.0f);

		if (changed)
		{
			for (int i = 0; i < 3; i++)
			{
				deg[i] = fmodf(deg[i], 360.0f);
				if (deg[i] < 0.0f) deg[i] += 360.0f;
				rotRadians[i] = deg[i] * (Math::PI / 180.0f);
			}
		}
		return changed;
	}

	bool EditorLayer::Knob(const char* label, float* value, float v_min, float v_max, float radius) {
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		float diameter = radius * 2.0f;
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		ImVec2 center = ImVec2(cursorPos.x + radius, cursorPos.y + radius);

		ImGui::PushID(label);
		ImGui::InvisibleButton("##knob", ImVec2(diameter, diameter));
		bool changed = false;

		bool isActive = ImGui::IsItemActive();
		bool isHovered = ImGui::IsItemHovered();
		
		if (isActive && io.MouseDelta.y != 0.0f)
		{
			float t = (*value - v_min) / (v_max - v_min);
			t = ImClamp(t - io.MouseDelta.y * 0.005f, 0.0f, 1.0f);
			*value = v_min + t * (v_max - v_min);
			changed = true;
		}
		
		float t = (*value - v_min) / (v_max - v_min);
		const float angleMax = Math::PI * 0.75f;
		const float angleMin = Math::PI * 2.25f;
		float angle = angleMin + t * (angleMax - angleMin);

		ImU32 colBase = ImGui::GetColorU32(isActive ? ImGuiCol_FrameBgActive :
			(isHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
		ImU32 colBorder = ImGui::GetColorU32(ImGuiCol_Border);
		ImU32 colIndicator = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);

		drawList->AddCircleFilled(center, radius, colBase, 32);
		drawList->AddCircle(center, radius, colBorder, 32, 1.5f);

		ImVec2 indicatorEnd(
			center.x + cosf(angle) * radius * 0.8f,
			center.y + sinf(angle) * radius * 0.8f
		);
		drawList->AddLine(center, indicatorEnd, colIndicator, 2.5f);

		if (isHovered || isActive)
			ImGui::SetTooltip("%.3f", *value);
		
		ImGui::TextUnformatted(label);
		ImGui::PopID();

		return changed;
	}
}