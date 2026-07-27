#include "archpch.h"
#include "Shader.h"

namespace Engine{

	std::shared_ptr<Shader> Shader::Create(ID3D11Device* device, const std::wstring& path) {
        auto shader = std::make_shared<Shader>();
        shader->_Path = path;

        if (!shader->Compile(device, shader->_VS, shader->_PS, shader->_Layout)) {
            return nullptr; // initial load failing is a hard failure, unlike Reload
        }
        return shader;
	}

    bool Shader::Reload(ID3D11Device* device) {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> newVS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> newPS;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> newLayout;
        
        if (!Compile(device, newVS, newPS, newLayout)) {
            LOG_WARN("Shader: Reload Failed, keeping previous working version");
            return false;
        }
        _VS = newVS;
        _PS = newPS;
        _Layout = newLayout;

        return true;
    }

    bool Shader::Compile(ID3D11Device* device,
        Microsoft::WRL::ComPtr<ID3D11VertexShader>& outVS,
        Microsoft::WRL::ComPtr<ID3D11PixelShader>& outPS,
        Microsoft::WRL::ComPtr<ID3D11InputLayout>& outLayout)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
        HRESULT hr;

        hr = D3DCompileFromFile(_Path.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) LOG_ERROR("Shader Compilation failed (VS): %s", (char*)errorBlob->GetBufferPointer());
            else LOG_ERROR("Shader compile failed (VS), no error blob. HRESULT: 0x%08X", hr);
            return false;
        }

        hr = D3DCompileFromFile(_Path.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &outVS);
        if (FAILED(hr)) { LOG_ERROR("CreateVertexShader failed. HRESULT: 0x%08X", hr); return false; }

        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &outPS);
        if (FAILED(hr)) { LOG_ERROR("CreatePixelShader failed. HRESULT: 0x%08X", hr); return false; }

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0}, 
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &outLayout);
        if (FAILED(hr)) { LOG_ERROR("CreateInputLayout failed. HRESULT: 0x%08X", hr); return false; }

        return true;
    }

}