#include "archpch.h"
#include "Texture.h"
#include <stb/stb_image.h>

namespace Engine {

	std::shared_ptr<Texture2D> Texture2D::Create(ID3D11Device* device, const std::string& path) {
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true); // matches typical UV convention (0,0 = top-left in D3D)

		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (!data) {
			LOG_ERROR("Texture2D::Create failed to load: %s | stb_image reason %s", path.c_str(), stbi_failure_reason());
			return nullptr;
		}

		auto texture = std::make_shared<Texture2D>();
		texture->_Width = (uint32_t)width;
		texture->_Height = (uint32_t)height;

		D3D11_TEXTURE2D_DESC td = {};
		td.Width = width;
		td.Height = height;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_IMMUTABLE;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = data;
		sd.SysMemPitch = width * 4;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d;
		HRESULT hr = device->CreateTexture2D(&td, &sd, &tex2d);
		stbi_image_free(data);
		if (FAILED(hr)) {
			LOG_ERROR("Texture::Create at CreateTexture2D failed; HRESULT: 0x%08X", hr);
			return nullptr;
		}

		hr = device->CreateShaderResourceView(tex2d.Get(), nullptr, &texture->_SRV);
		if (FAILED(hr)) {
			LOG_ERROR("Texture::Create at CreateShaderResourceView failed; HRESULT: 0x%08X", hr);
			return nullptr;
		}

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		hr = device->CreateSamplerState(&samplerDesc, &texture->_Sampler);
		if (FAILED(hr)) {
			LOG_ERROR("Texture::Create at CreateSamplerState failed; HRESULT: 0x%08X", hr);
			return nullptr;
		}

		return texture;
	}

	std::shared_ptr<Texture2D> Texture2D::CreateSolidColor(ID3D11Device* device, uint32_t rgba) {
		auto texture = std::make_shared<Texture2D>();
		texture->_Width = 1;
		texture->_Height = 1;

		D3D11_TEXTURE2D_DESC td = {};
		td.Width = 1;
		td.Height = 1;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_IMMUTABLE;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		
		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = &rgba; // packed as 0xAABBGGRR in memory order (R,G,B,A bytes)
		sd.SysMemPitch = 4;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d;
		HRESULT hr = device->CreateTexture2D(&td, &sd, &tex2d);
		if (FAILED(hr)) { 
			LOG_ERROR("Texture::Create at CreateTexture2D failed; HRESULT: 0x%08X", hr);
			return nullptr; 
		}

		hr = device->CreateShaderResourceView(tex2d.Get(), nullptr, &texture->_SRV);
		if (FAILED(hr)) { 
			LOG_ERROR("Texture::Create at CreateShaderResourceView failed; HRESULT: 0x%08X", hr);
			return nullptr; 
		}

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // point sampling, no filtering needed for 1x1
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		hr = device->CreateSamplerState(&samplerDesc, &texture->_Sampler);
		if (FAILED(hr)) { 
			LOG_ERROR("Texture::Create at CreateSamplerState failed; HRESULT: 0x%08X", hr);
			return nullptr; 
		}

		return texture;
	}
}