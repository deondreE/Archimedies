#pragma once
#include "archpch.h"

namespace Engine {

	class Texture2D {
	public:
		static std::shared_ptr<Texture2D> Create(ID3D11Device* device, const std::string& path, bool flip_vertically = true);
		static std::shared_ptr<Texture2D> CreateSolidColor(ID3D11Device* device, uint32_t rgba);
		static std::shared_ptr<Texture2D> CreateFromRGBA(ID3D11Device* device, uint32_t width, uint32_t height, const uint8_t* pixels);
		static std::shared_ptr<Texture2D> GetDefaultTexture(ID3D11Device* device);

		ID3D11ShaderResourceView* GetSRV() const { return _SRV.Get(); }
		ID3D11SamplerState* GetSampler() const { return _Sampler.Get(); }

		uint32_t GetWidth() const { return _Width; }
		uint32_t GetHeight() const { return _Height; }
	private:
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _SRV;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _Sampler;
		uint32_t _Width = 0;
		uint32_t _Height = 0;
	};
}