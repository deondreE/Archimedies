#include "archpch.h"
#include "Application.h"
#include "Renderer.h"
#include "Input.h"
#include "WindowEvents.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "Event.h"
#include "Math/TestMath.h"
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine {
	Application::Application(const ApplicationSpecification& spec) 
	: _Specification(spec), _Width((int)spec.Width), _Height((int)spec.Height) {
		_ActiveScene = std::make_unique<Scene>();

		if (!_Specification.WorkingDirectory.empty()) {
			SetCurrentDirectoryW(_Specification.WorkingDirectory.c_str());
		}

		// Only run when I want it to
#if 0
		Math::RunMathTests();
#endif	
	}

	Application::~Application() {
#if ARCH_RENDERER_D3D12
		if (_Device) WaitForGpu();
		if (_FenceEvent) CloseHandle(_FenceEvent);
#endif
	}

	bool Application::InitWindow()
	{
		WNDCLASSEXW wc = { 0 };
		wc.cbSize = sizeof(wc);
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = L"ArchimediesWindowClass";
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		RegisterClassExW(&wc);

		_HWnd = CreateWindowExW(0, wc.lpszClassName, _Specification.Name.c_str(),
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, _Width, _Height,
			nullptr, nullptr, wc.hInstance, this);
		RegisterClassExW(&wc);

		if (_HWnd) Input::Init(_HWnd);

		return _HWnd != nullptr;
	}

#if ARCH_RENDERER_DX12
	// ------------------------------------------------------------------
	// D3D12 path
	// ------------------------------------------------------------------
	bool Application::CreateDepthStencil(uint32_t width, uint32_t height) {
		if (!_DSVHeap) {
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			HRESULT hr = _Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_DSVHeap));
			if (FAILED(hr)) {
				LOG_ERROR("CreateDescriptorHeap (DSV) failed. HRESULT: 0x%08X", hr);
				return false;
			}
		}

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC depthDesc = {};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Width = width;
		depthDesc.Height = height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthDesc.SampleDesc.Count = 1;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		_DepthStencilBuffer.Reset();
		HRESULT hr = _Device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
			IID_PPV_ARGS(&_DepthStencilBuffer));
		if (FAILED(hr)) {
			LOG_ERROR("CreateCommittedResource (depth) failed. HRESULT: 0x%08X", hr);
			return false;
		}

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		_Device->CreateDepthStencilView(_DepthStencilBuffer.Get(), &dsvDesc, _DSVHeap->GetCPUDescriptorHandleForHeapStart());

		return true;
	}

	void Application::WaitForGpu() {
		const UINT64 fenceValueToWaitFor = ++_FenceValue;
		_CommandQueue->Signal(_Fence.Get(), fenceValueToWaitFor);

		if (_Fence->GetCompletedValue() < fenceValueToWaitFor) {
			_Fence->SetEventOnCompletion(fenceValueToWaitFor, _FenceEvent);
			WaitForSingleObject(_FenceEvent, INFINITE);
		}
	}

	bool Application::InitDx()
	{
		UINT dxgiFactoryFlags = 0;
#ifdef DEBUG
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif

		Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
		HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
		if (FAILED(hr)) {
			LOG_ERROR("CreateDXGIFactory2 failed. HRESULT: 0x%08X", hr);
			return false;
		}

		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));

		hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_Device));
		if (FAILED(hr)) {
			LOG_ERROR("D3D12CreateDevice failed. HRESULT: 0x%08X", hr);
			return false;
		}

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		hr = _Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_CommandQueue));
		if (FAILED(hr)) {
			LOG_ERROR("CreateCommandQueue failed. HRESULT: 0x%08X", hr);
			return false;
		}

		DXGI_SWAP_CHAIN_DESC1 scd = {};
		scd.Width = _Width;
		scd.Height = _Height;
		scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.SampleDesc.Count = 1;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = FrameCount;
		scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
		hr = factory->CreateSwapChainForHwnd(_CommandQueue.Get(), _HWnd, &scd, nullptr, nullptr, &swapChain1);
		if (FAILED(hr)) {
			LOG_ERROR("CreateSwapChainForHwnd failed. HRESULT: 0x%08X", hr);
			return false;
		}
		swapChain1.As(&_SwapChain);
		_FrameIndex = _SwapChain->GetCurrentBackBufferIndex();

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		hr = _Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_RTVHeap));
		if (FAILED(hr)) {
			LOG_ERROR("CreateDescriptorHeap (RTV) failed. HRESULT: 0x%08X", hr);
			return false;
		}
		_RTVDescriptorSize = _Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _RTVHeap->GetCPUDescriptorHandleForHeapStart();
		for (uint32_t i = 0; i < FrameCount; i++) {
			hr = _SwapChain->GetBuffer(i, IID_PPV_ARGS(&_RenderTargets[i]));
			if (FAILED(hr)) {
				LOG_ERROR("SwapChain GetBuffer(%u) failed. HRESULT: 0x%08X", i, hr);
				return false;
			}
			_Device->CreateRenderTargetView(_RenderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += _RTVDescriptorSize;
		}

		for (uint32_t i = 0; i < FrameCount; i++) {
			hr = _Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_CommandAllocators[i]));
			if (FAILED(hr)) {
				LOG_ERROR("CreateCommandAllocator(%u) failed. HRESULT: 0x%08X", i, hr);
				return false;
			}
		}

		hr = _Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _CommandAllocators[_FrameIndex].Get(),
			nullptr, IID_PPV_ARGS(&_CommandList));
		if (FAILED(hr)) {
			LOG_ERROR("CreateCommandList failed. HRESULT: 0x%08X", hr);
			return false;
		}
		_CommandList->Close(); // Run() expects the list closed at the top of each frame

		if (!CreateDepthStencil((uint32_t)_Width, (uint32_t)_Height)) return false;

		hr = _Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_Fence));
		if (FAILED(hr)) {
			LOG_ERROR("CreateFence failed. HRESULT: 0x%08X", hr);
			return false;
		}
		_FenceValue = 0;
		_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!_FenceEvent) {
			LOG_ERROR("CreateEvent (fence) failed. GetLastError: %lu", GetLastError());
			return false;
		}

		// ImGui's DX12 backend needs a small SRV heap of its own (font texture + any user textures it manages).
		D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc = {};
		imguiHeapDesc.NumDescriptors = 64;
		imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = _Device->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&_ImGuiSrvHeap));
		if (FAILED(hr)) {
			LOG_ERROR("CreateDescriptorHeap (ImGui SRV) failed. HRESULT: 0x%08X", hr);
			return false;
		}

		return true;
	}

#else
	bool Application::CreateDepthStencil(uint32_t width, uint32_t height) {
		D3D11_TEXTURE2D_DESC dsd = {};
		dsd.Width = width;
		dsd.Height = height;
		dsd.MipLevels = 1;
		dsd.ArraySize = 1;
		dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsd.SampleDesc.Count = 1;
		dsd.SampleDesc.Quality = 0;
		dsd.Usage = D3D11_USAGE_DEFAULT;
		dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		HRESULT hr = _Device->CreateTexture2D(&dsd, nullptr, &_DepthStencilBuffer);
		if (FAILED(hr)) return false;

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
		dsvd.Format = dsd.Format;
		dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvd.Texture2D.MipSlice = 0;

		hr = _Device->CreateDepthStencilView(_DepthStencilBuffer.Get(), &dsvd, &_DepthStencilView);
		return SUCCEEDED(hr);
	}

	bool Application::InitDx()
	{
		DXGI_SWAP_CHAIN_DESC  sd = {};
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 0;
		sd.BufferDesc.RefreshRate.Denominator = 0;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 1;
		sd.OutputWindow = _HWnd;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		UINT createDeviceFlags = 0;
#ifdef DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		HRESULT hr;
		hr = D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
			nullptr, 0, D3D11_SDK_VERSION, &sd,
			&_SwapChain, &_Device, nullptr, &_Context
		);
		if (FAILED(hr)) {
			LOG_ERROR("D3D11CreateDeviceAndSwapChain failed. HRESULT: 0x%08X", hr);
			return false;
		}

		Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
		_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_RenderTargetView);

		if (!CreateDepthStencil((uint32_t)_Width, (uint32_t)_Height)) return false;

		D3D11_RASTERIZER_DESC rd = {};
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_BACK;
		rd.FrontCounterClockwise = FALSE;
		rd.DepthClipEnable = TRUE;

		hr = _Device->CreateRasterizerState(&rd, &_RasterizerState);
		if (FAILED(hr)) {
			LOG_ERROR("CreateRasterizerState failed. HRESULT: 0x%08X", hr);
			return false;
		}

		_Context->RSSetState(_RasterizerState.Get());

		D3D11_VIEWPORT vp{  };
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<float>(_Width);
		vp.Height = static_cast<float>(_Height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		_Context->RSSetViewports(1, &vp);

		_Context->OMSetRenderTargets(1, _RenderTargetView.GetAddressOf(), _DepthStencilView.Get());

		return true;
	}
#endif

	// Layers
	void Application::PushLayer(Layer* layer) {
		_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* layer) {
		_LayerStack.PushOverlay(layer);
	}

	void Application::OnEvent(Event& e) {
		// Reverse iteration: topmost layer (overlays, e.g. future ImGui) sees the event FIRST,
		// and can mark it Handled to stop it propagating further down to the game layer.
		for (auto it = _LayerStack.rbegin(); it != _LayerStack.rend(); ++it) {
			if (e.Handled) break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run() {
		if (!InitWindow()) return;
		if (!InitDx()) return;

		Renderer::Init(_Device.Get(), _Context.Get(), _Specification);
		
		_ImGuiLayer = new ImGuiLayer(_HWnd, _Device.Get(), _Context.Get());
		PushOverlay(_ImGuiLayer);
		OnInit();
		

		LARGE_INTEGER frequency, lastTime;
		QueryPerformanceFrequency(&frequency);
		double invFrequency = 1.0 / static_cast<double>(frequency.QuadPart);
		QueryPerformanceCounter(&lastTime);

		float shaderCheckTimer = 0.0f;
		constexpr float shaderCheckInterval = 0.5f; // twice a second plenty for manual edit.

		MSG msg = { 0 };
		while (_Running) {
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) _Running = false;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			LARGE_INTEGER currentTime;
			QueryPerformanceCounter(&currentTime);
			double deltaSeconds = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) * invFrequency;
			lastTime = currentTime;
			
			if (deltaSeconds > 0.1) deltaSeconds = 0.1;
			if (deltaSeconds < 0.0) deltaSeconds = 0.0; // Guard against rare QPC jitter

			Timestep ts(static_cast<float>(deltaSeconds));
			static float accumulator = 0.0f;
			Timestep fixedTS = Timestep::Fixed(60.0f);

			accumulator += ts.GetSeconds();

			while (accumulator >= fixedTS) {
				OnFixedUpdate(fixedTS);
				accumulator -= fixedTS;
			}

			shaderCheckTimer += ts.GetSeconds();
			if (shaderCheckTimer >= shaderCheckInterval) {
				shaderCheckTimer = 0.0f;
				Renderer::GetShaderLibrary().CheckForChanges(_Device.Get());
			}

			float alpha = Timestep::GetAlpha(accumulator, fixedTS);
			// OnRender(alpha);

			OnUpdate(ts);

			for (Layer* layer : _LayerStack) {
				layer->OnUpdate(ts);
			}

			float clearColor[] = { 0.1f, 0.15f, 0.2f, 1.0f };
			_Context->ClearRenderTargetView(_RenderTargetView.Get(), clearColor);
			_Context->ClearDepthStencilView(_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			
			OnRender();

			for (Layer* layer : _LayerStack) {
				if (layer == _ImGuiLayer) continue;
				layer->OnRender();
			}

			// ImGui frame wraps around here: Begin() before any ImGui:: calls, End() after all of them
			_ImGuiLayer->Begin();
			_ImGuiLayer->BeginDocking();
			for (Layer* layer : _LayerStack) {
				layer->OnMenuBarRender();
			}
			_ImGuiLayer->EndMenuBar();
			for (Layer* layer : _LayerStack) {
				layer->OnImGuiRender(); 
			}
			_ImGuiLayer->EndDocking();
			_ImGuiLayer->End();


			_SwapChain->Present(1, 0); // VSync
		}

		Renderer::Shutdown();
	}
	
	LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) // now resolves to ::ImGui_ImplWin32_WndProcHandler correctly, since ADL/lookup finds it in the enclosing global scope
			return true;

		if (uMsg == WM_NCCREATE) {
			auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			auto* app = reinterpret_cast<Application*>(cs->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
		}

		auto* app = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (!app) return DefWindowProc(hWnd, uMsg, wParam, lParam);

		switch (uMsg) {
		case WM_DESTROY: {
			WindowCloseEvent e;
			app->OnEvent(e);
			PostQuitMessage(0);
			return 0;
		}
		case WM_SIZE: {
			if (wParam != SIZE_MINIMIZED) {
				uint32_t width = LOWORD(lParam);
				uint32_t height = HIWORD(lParam);
				WindowResizeEvent e(width, height);
				app->OnWindowResize(e); // internal: recreate buffers first
				app->OnEvent(e);        // then let the derived app react (camera aspect, etc.)
			}
			return 0;
		}
		case WM_KEYDOWN: {
			bool isRepeat = (lParam & 0x40000000) != 0; // bit 30: key was already down
			KeyPressedEvent e((int)wParam, isRepeat);
			app->OnEvent(e);
			return 0;
		}
		case WM_KEYUP: {
			KeyReleasedEvent e((int)wParam);
			app->OnEvent(e);
			return 0;
		}
		case WM_LBUTTONDOWN: { MouseButtonPressedEvent e(VK_LBUTTON); app->OnEvent(e); return 0; }
		case WM_LBUTTONUP: { MouseButtonReleasedEvent e(VK_LBUTTON); app->OnEvent(e); return 0; }
		case WM_RBUTTONDOWN: { MouseButtonPressedEvent e(VK_RBUTTON); app->OnEvent(e); return 0; }
		case WM_RBUTTONUP: { MouseButtonReleasedEvent e(VK_RBUTTON); app->OnEvent(e); return 0; }
		case WM_MOUSEMOVE: {
			float x = (float)GET_X_LPARAM(lParam);
			float y = (float)GET_Y_LPARAM(lParam);
			MouseMovedEvent e(x, y);
			app->OnEvent(e);
			return 0;
		}
		case WM_MOUSEWHEEL: {
			float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			MouseScrolledEvent e(delta);
			app->OnEvent(e);
			return 0;
		}
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	// @AI fix
	void Application::OnWindowResize(WindowResizeEvent& e) {
		uint32_t width = e.GetWidth();
		uint32_t height = e.GetHeight();

		if (width == 0 || height == 0) return;             // minimized, ignore
		if ((int)width == _Width && (int)height == _Height) return;

		_Width = (int)width;
		_Height = (int)height;

		if (!_SwapChain) return; // can fire before InitDx() completes

		// Must release everything referencing the back buffer before ResizeBuffers
		_RenderTargetView.Reset();
		_DepthStencilView.Reset();
		_DepthStencilBuffer.Reset();
		_Context->OMSetRenderTargets(0, nullptr, nullptr);

		HRESULT hr = _SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr)) {
			LOG_ERROR("ResizeBuffers failed. HRESULT: 0x%08X", hr);
			return;
		}

		Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
		_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &_RenderTargetView);

		if (!CreateDepthStencil(width, height)) return;

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		_Context->RSSetViewports(1, &vp);

		_Context->OMSetRenderTargets(1, _RenderTargetView.GetAddressOf(), _DepthStencilView.Get());
	}
}
