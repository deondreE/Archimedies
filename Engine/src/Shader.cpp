#include "archpch.h"
#include "Shader.h"

namespace Engine{

	std::shared_ptr<Shader> Shader::Create(ID3D11Device* device, const std::wstring& path) {
		auto shader = std::make_shared<Shader>();

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
		HRESULT hr;

        hr = D3DCompileFromFile(path.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) LOG_ERROR("Shader Compilation failed (VS): %s", (char)errorBlob->GetBufferPointer());
            else LOG_ERROR("Shader compile failed (VS), no error blob. HRESULT: 0x%08X", hr);
            return nullptr;
        }

        hr = D3DCompileFromFile(path.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return nullptr;
        }
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &shader->_VS);
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &shader->_PS);

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &shader->_Layout);
        
        return shader;
	}

}