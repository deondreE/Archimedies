#pragma once
#include "archpch.h"

namespace Engine {
	class Shader {
	public:
		static std::shared_ptr<Shader> Create(ID3D11Device* device, const std::wstring& path);

		ID3D11VertexShader* GetVS() const { return _VS.Get(); }
		ID3D11PixelShader* GetPS() const { return _PS.Get(); }
		ID3D11InputLayout* GetLayout() const { return _Layout.Get(); }
	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> _VS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> _PS;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> _Layout;
	};
}
