#include "archpch.h"
#include "Application.h"
#include "Renderer.h"
#include "Input.h"
#include "WindowEvents.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "Event.h"
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine {
	Application::Application(const ApplicationSpecification& spec) 
	: _Specification(spec), _Width((int)spec.Width), _Height((int)spec.Height) {
		_ActiveScene = std::make_unique<Scene>();

		if (!_Specification.WorkingDirectory.empty()) {
			SetCurrentDirectoryW(_Specification.WorkingDirectory.c_str());
		}
	}

	Application::~Application() {}

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
		

		LARGE_INTEGER frequency, lastTime, currentTime;
		QueryPerformanceFrequency(&frequency);
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

			QueryPerformanceCounter(&currentTime);
			float deltaSeconds = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / static_cast<float>(frequency.QuadPart);
			lastTime = currentTime;
			
			// Defend against large spiks in time: (Resume after breakpoint, Window Drag)
			Timestep ts(std::min(deltaSeconds, 0.1f));

			shaderCheckTimer += ts.GetSeconds();
			if (shaderCheckTimer >= shaderCheckInterval) {
				shaderCheckTimer = 0.0f;
				Renderer::GetShaderLibrary().CheckForChanges(_Device.Get());
			}

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
				layer->OnImGuiRender(); // new virtual — see Layer.h update below
			}
			_ImGuiLayer->EndDocking();
			_ImGuiLayer->End();


			_SwapChain->Present(1, 0); // VSync
		}

		//for (Layer* layer : _LayerStack) {
		//	layer->OnDetach();
		//}

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

	void Application::OnResize(uint32_t width, uint32_t height) {
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
		if (FAILED(hr)) return;

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

		_Context->OMSetRenderTargets(1, _RenderTargetView.GetAddressOf(), nullptr);

		OnViewportResize(width, height);
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
