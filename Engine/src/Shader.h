#pragma once
#include "archpch.h"

namespace Engine {
	// @TODO: Shader crashes on Undefined Behaviour.
	class Shader {
	public:
		static std::shared_ptr<Shader> Create(ID3D11Device* device, const std::wstring& path);

		bool Reload(ID3D11Device* device);

		ID3D11VertexShader* GetVS() const { return _VS.Get(); }
		ID3D11PixelShader* GetPS() const { return _PS.Get(); }
		ID3D11InputLayout* GetLayout() const { return _Layout.Get(); }

		const std::wstring& GetPath() const { return _Path; }
	private:
		bool Compile(ID3D11Device* device,
			Microsoft::WRL::ComPtr<ID3D11VertexShader>& outVS,
			Microsoft::WRL::ComPtr<ID3D11PixelShader>& outPS,
			Microsoft::WRL::ComPtr<ID3D11InputLayout>& outLayout);

		Microsoft::WRL::ComPtr<ID3D11VertexShader> _VS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> _PS;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> _Layout;
		std::wstring _Path;
	};
}
