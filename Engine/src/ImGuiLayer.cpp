#include "archpch.h"
#include "ImGuiLayer.h"

namespace Engine {
	ImGuiLayer::ImGuiLayer(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
		: Layer("ImGui"), _HWnd(hwnd), _Device(device), _Context(context) {
	}
	ImGuiLayer::~ImGuiLayer() {}

	void ImGuiLayer::OnAttach() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(_HWnd);
		ImGui_ImplDX11_Init(_Device, _Context);
	}

	void ImGuiLayer::Begin() {
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End() {
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	void ImGuiLayer::OnEvent(Event& e) {
		// If ImGui wants keyboard/mouse focus (e.g. a text box, or the mouse is over a window),
		// mark the event handled so it doesn't fall through to the game layer underneath.
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
			e.Handled = true;
		}
	}

	void ImGuiLayer::OnDetach() {
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}