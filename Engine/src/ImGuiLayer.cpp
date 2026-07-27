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
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

	void ImGuiLayer::BeginDocking() {
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

		ImGui::Begin("DockSpaceHost", nullptr, windowFlags);
		//ImGui::PopStyleVar(1);

		ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Exit")) PostQuitMessage(0);
				ImGui::EndMenu();
			}
		}
	}

	void ImGuiLayer::EndMenuBar() {
		ImGui::EndMenuBar();
	}

	void ImGuiLayer::EndDocking() {
		ImGui::End();
	}

	void ImGuiLayer::OnDetach() {
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}