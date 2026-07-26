#pragma once
#include "Layer.h"

namespace Engine {
	
	class ImGuiLayer : public Layer {
	public:
		ImGuiLayer(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
		virtual ~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();
	private:
		HWND _HWnd;
		ID3D11Device* _Device;
		ID3D11DeviceContext* _Context;
	};
}