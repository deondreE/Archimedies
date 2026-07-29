#include "archpch.h"
#include "Mesh.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace Engine {

    std::shared_ptr<Mesh> Mesh::Create(ID3D11Device* device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::string& path)
    {
        auto mesh = std::make_shared<Mesh>();
        mesh->_FilePath = path;
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

    std::shared_ptr<Mesh> Mesh::LoadFromFile(ID3D11Device* device, const std::string& filepath) {
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool isBinary = filepath.substr(filepath.find_last_of(".") + 1) == "glb";
        bool success = isBinary ?
            loader.LoadBinaryFromFile(&model, &err, &warn, filepath) :
            loader.LoadASCIIFromFile(&model, &err, &warn, filepath);

        if (!success) {
            LOG_ERROR("TinyGLTF Error: %s", err.c_str());
            return nullptr;
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        // Direct access to the first mesh and primitive
        const auto& mesh = model.meshes[0];
        const auto& primitive = mesh.primitives[0];

        // 1. Extract Positions
        const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
        const auto& posBufferView = model.bufferViews[posAccessor.bufferView];
        const auto& posBuffer = model.buffers[posBufferView.buffer];
        const float* posData = reinterpret_cast<const float*>(&(posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]));
        vertices.resize(posAccessor.count);

        // 2. Extract Normals (if they exist)
        const float* normalData = nullptr;
        if (primitive.attributes.count("NORMAL")) {
            const auto& acc = model.accessors[primitive.attributes.at("NORMAL")];
            const auto& view = model.bufferViews[acc.bufferView];
            normalData = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]));
        }

        // 3. Extract TexCoords (UVs)
        const float* uvData = nullptr;
        if (primitive.attributes.count("TEXCOORD_0")) {
            const auto& acc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
            const auto& view = model.bufferViews[acc.bufferView];
            uvData = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]));
        }

        // Interleave data into your Vertex struct
        for (size_t i = 0; i < posAccessor.count; ++i) {
            vertices[i].Position = { posData[i * 3], posData[i * 3 + 1], posData[i * 3 + 2] };

            // Default color to white so the shader's (texColor * col) pass-through works correctly.
            vertices[i].Color[0] = 1.0f;
            vertices[i].Color[1] = 1.0f;
            vertices[i].Color[2] = 1.0f;
            vertices[i].Color[3] = 1.0f;

            if (normalData) {
                vertices[i].Normal[0] = normalData[i * 3];
                vertices[i].Normal[1] = normalData[i * 3 + 1];
                vertices[i].Normal[2] = normalData[i * 3 + 2];
            } else {
                // Default to up so lighting isn't zero when normals are absent.
                vertices[i].Normal[0] = 0.0f;
                vertices[i].Normal[1] = 1.0f;
                vertices[i].Normal[2] = 0.0f;
            }

            if (uvData) {
                vertices[i].UV[0] = uvData[i * 2];
                vertices[i].UV[1] = uvData[i * 2 + 1];
            }
        }

        // 4. Extract Indices
        const auto& indexAccessor = model.accessors[primitive.indices];
        const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const auto& indexBuffer = model.buffers[indexBufferView.buffer];
        const void* dataPtr = &(indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);

        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
            indices.assign(buf, buf + indexAccessor.count);
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
            for (size_t i = 0; i < indexAccessor.count; ++i) indices.push_back(buf[i]);
        }

        return Create(device, vertices, indices, filepath);
    }
}