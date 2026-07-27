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
		virtual void OnRender() {}
		virtual void OnEvent(Event& e);
		virtual void OnViewportResize(uint32_t width, uint32_t height) {}

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		const ApplicationSpecification& GetSpecification() const { return _Specification; }

#if ARCH_RENDERER_D3D12
		ID3D12Device* GetDevice() const { return _Device.Get(); }
		ID3D12GraphicsCommandList* GetCommandList() const { return _CommandList.Get(); }
		ID3D12CommandQueue* GetCommandQueue() const { return _CommandQueue.Get(); }
#else
		ID3D11Device* GetDevice() const { return _Device.Get(); }
		ID3D11DeviceContext* GetContext() const { return _Context.Get(); }
#endif
	protected:
		std::unique_ptr<Scene> _ActiveScene;

		bool InitWindow();
		bool InitDx();
		// void OnResize(uint32_t width, uint32_t height);

		void OnWindowResize(WindowResizeEvent& e);

		std::wstring _Name;
		ApplicationSpecification _Specification;
		bool _Running = true;
		int _Width = 1280;
		int _Height = 720;

		HWND _HWnd = nullptr;

#if ARCH_RENDERER_D3D12
		static constexpr uint32_t _FrameCount = 2;

		Microsoft::WRL::ComPtr<ID3D12Device> _Device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> _CommandQueue;
		Microsoft::WRL::ComPtr<IDXGISwapChain> _SwapChain;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _RTVHeap;
		uint32_t _RTVDescriptorSize = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> _RenderTargets[_FrameCount];

		// One allocator per in-flight frame so we never reset one the GPU is still reading from.
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _CommandAllocator[_FrameCount];
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _CommandList;

		// Simple whole-GPU-wait sync (not a true 2-frame pipeline yet).
		// Good enough to get D3D12 rendering correctly; revisit if you need the perf.
		Microsoft::WRL::ComPtr<ID3D12Fence> _Fence;
		HANDLE _FenceEvent = nullptr;
		UINT64 _FenceValue = 0;
		uint32_t _FrameIndex = 0;

		// ImGui needs its own small SRV heap on D3D12 (font texture lives here).
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _ImGuiSrvHeap;

		void WaitForGpu();
		bool CreateDepthStencil(uint32_t width, uint32_t height); // (re)creates DSV heap + resource
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

		bool CreateDepthStencil(uint32_t width, uint32_t height);
		// void OnWindowResize(WindowResizeEvent& e);
	};
	
	// To be defined in Sandbox
	Application* CreateApplication();
}