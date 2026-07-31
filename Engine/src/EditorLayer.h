#pragma once
#include "Layer.h"
#include "ContentBrowser.h"
#include "Scene.h"

namespace Engine {

	class EditorLayer : public Layer {
	public:
		EditorLayer(Scene* scene, ID3D11Device* device, const fs::path& assetRoot) : 
			Layer("Edtior"), _Scene(scene), _ContentBrowser(device, assetRoot), _Device(device) {}

		virtual void OnMenuBarRender() override;
		virtual void OnImGuiRender() override;
	private:
		bool DragRotationDeg3(const char* label, float* rotRadians, float v_speed = 0.5f);
		bool DragFloat3Colored(const char* label, float* v, float v_speed,
			float v_min, float v_max, const char* format = "%.1f");
		bool Knob(const char* label, float* value, float v_min, float v_max, float radius = 20.0f);
		Scene* _Scene;
		int _SelectedIndex = -1;

		ContentBrowserPanel _ContentBrowser;
		ID3D11Device* _Device;
		bool _ShowContentBrowser = true;
		bool _ShowHierarchy = true;
		bool _ShowInspector = true;
		bool _ShowLighting = true; 
		bool _ShowStats = false; // @TODO: Show engine stats here. 
	};
}