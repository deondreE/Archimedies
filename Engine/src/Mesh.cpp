#include "archpch.h"
#include "Mesh.h"

namespace Engine {

    std::shared_ptr<Mesh> Mesh::Create(ID3D11Device* device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices)
    {
        auto mesh = std::make_shared<Mesh>();
        HRESULT hr;

        D3D11_BUFFER_DESC vbd = {};
        vbd.Usage = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vsd = {};
        vsd.pSysMem = vertices.data();
        hr = device->CreateBuffer(&vbd, &vsd, &mesh->_VertexBuffer);
        if (FAILED(hr)) {
            LOG_ERROR("Mesh::Create failed to create vertex buffer. HRESULT: 0x%08X", hr);
            return nullptr;
        }

        D3D11_BUFFER_DESC ibd = {};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = (UINT)(indices.size() * sizeof(uint32_t));
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA isd = {};
        isd.pSysMem = indices.data();
        hr = device->CreateBuffer(&ibd, &isd, &mesh->_IndexBuffer);
        if (FAILED(hr)) {
            LOG_ERROR("Mesh::Create failed to create vertex buffer. HRESULT: 0x%08X", hr);
            return nullptr;
        }

        mesh->_IndexCount = (uint32_t)indices.size();
        return mesh;
    }
}