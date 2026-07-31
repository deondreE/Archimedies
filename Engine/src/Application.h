#pragma once
#include "archpch.h"
#include "Scene.h"
#include "Timestep.h"
#include "Event.h"
#include "WindowEvents.h"
#include "LayerStack.h"
#include "ImGuiLayer.h"

namespace Engine {

	struct ApplicationSpecification {
		std::wstring Name = L"Engine Application";
		std::wstring WorkingDirectory = L""; // E.g: "../Sandbox";
		uint32_t Width = 1280;
		uint32_t Height = 720;
	};

	class Application {
	public:
		Application(const ApplicationSpecification& spec);
		virtual ~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void Run();
		virtual void OnInit() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnFixedUpdate(Timestep ts) {}
		virtual void OnRender() {}
		virtual void OnEvent(Event& e);
		virtual void OnViewportResize(uint32_t width, uint32_t height) {}
		
		EditorMode GetEditorMode() const { return _Mode; }
		void SetEditorMode(EditorMode mode) { _Mode = mode; }

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		const ApplicationSpecification& GetSpecification() const { return _Specification; }

#if ARCH_RENDERER_D3D12
#else
		ID3D11Device* GetDevice() const { return _Device.Get(); }
		ID3D11DeviceContext* GetContext() const { return _Context.Get(); }
#endif
	protected:
		std::unique_ptr<Scene> _ActiveScene;

		bool InitWindow();
		bool InitDx();

		void OnWindowResize(WindowResizeEvent& e);

		std::wstring _Name;
		ApplicationSpecification _Specification;
		bool _Running = true;
		int _Width = 1280;
		int _Height = 720;

		HWND _HWnd = nullptr;

#if ARCH_RENDERER_D3D12
#else
		Microsoft::WRL::ComPtr<ID3D11Device> _Device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _Context;
		Microsoft::WRL::ComPtr<IDXGISwapChain> _SwapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _RenderTargetView;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> _DepthStencilBuffer;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _DepthStencilView;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> _RasterizerState;
#endif
		LayerStack _LayerStack;
		ImGuiLayer* _ImGuiLayer = nullptr;
	private:
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		EditorMode _Mode;

		bool CreateDepthStencil(uint32_t width, uint32_t height);
	};
	
	Application* CreateApplication();
}