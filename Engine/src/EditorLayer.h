#pragma once
#include "Layer.h"
#include "Scene.h"

namespace Engine {

	class EditorLayer : public Layer {
	public:
		EditorLayer(Scene* scene) : Layer("Edtior"), _Scene(scene) {}

		virtual void OnMenuBarRender() override;
		virtual void OnImGuiRender() override;
	private:
		Scene* _Scene;
		int _SelectedIndex = -1;
		bool _ShowHierarchy = true;
		bool _ShowInspector = true;
		bool _ShowLighting = true;
		bool _ShowStats = true;
	};
}