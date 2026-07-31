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

	void ImGuiLayer::BeginDocking(EditorMode& mode) {
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		//@Info: Make sure this never has NoInputs.
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

		ImGui::Begin("DockSpaceHost", nullptr, windowFlags);

		ImGui::PopStyleVar(3);

		ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Exit")) PostQuitMessage(0);
				ImGui::EndMenu();
			}

			float toolBarButtonSize = ImGui::GetFrameHeight() - 4.0f;
			int numButtons = 2; // Play and Stop
			float totalToolbarWidth = (toolBarButtonSize * numButtons) + ImGui::GetStyle().ItemSpacing.x;
			
			float centerX = (ImGui::GetWindowWidth() - totalToolbarWidth) * 0.5f;

			ImGui::SetCursorPosX(centerX);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

			bool isPlaying = (mode == EditorMode::Play); 

			if (isPlaying) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.35f, 0.58f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.70f, 1.0f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			}

			if (ImGui::Button(" > ", ImVec2(toolBarButtonSize, toolBarButtonSize))) {
				mode = isPlaying ? EditorMode::Default : EditorMode::Play;
			}
			ImGui::PopStyleColor(isPlaying ? 2 : 1);

			ImGui::SameLine();

			bool isDefault = (mode == EditorMode::Default);
			if (isDefault) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			}

			if (ImGui::Button(" # ", ImVec2(toolBarButtonSize, toolBarButtonSize))) {
				mode = EditorMode::Default;
			}
			ImGui::PopStyleColor();

			ImGui::SetCursorPosX(100);

			ImGui::PopStyleVar(2);
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