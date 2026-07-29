#include "archpch.h"
#include "Renderer.h"
#include "Application.h"

namespace Engine {
    
    Renderer::RendererData* Renderer::s_Data = nullptr;

    void Renderer::Init(ID3D11Device* device, ID3D11DeviceContext* context, const ApplicationSpecification& spec) {
        s_Data = new RendererData();
        s_Data->Device = device;
        s_Data->Context = context;

        HRESULT hr;
        D3D11_BUFFER_DESC sceneCbd = {};
        sceneCbd.Usage = D3D11_USAGE_DYNAMIC;
        sceneCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        sceneCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        sceneCbd.ByteWidth = sizeof(SceneConstants);

        hr =s_Data->Device->CreateBuffer(&sceneCbd, nullptr, &s_Data->SceneCB);
        if (FAILED(hr)) {
            LOG_ERROR("Renderer::Init failed to create SceneCB. HRESULT: 0x%08X, ByteWidth: %u", hr, sceneCbd.ByteWidth);
            return;
        }

        D3D11_BUFFER_DESC entityCbd = {};
        entityCbd.Usage = D3D11_USAGE_DYNAMIC;
        entityCbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        entityCbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        entityCbd.ByteWidth = sizeof(EntityConstants);
        
        hr = s_Data->Device->CreateBuffer(&entityCbd, nullptr, &s_Data->EntityCB);
        if (FAILED(hr)) {
            LOG_ERROR("Renderer::Init failed to create SceneCB. HRESULT: 0x%08X, ByteWidth: %u", hr, entityCbd.ByteWidth);
            return;
        }

        s_Data->WhiteTexture = Texture2D::CreateSolidColor(device, 0xFFFFFFFF);

        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable           = TRUE;
        blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = device->CreateBlendState(&blendDesc, &s_Data->AlphaBlendState);
        if (FAILED(hr)) {
            LOG_ERROR("Renderer::Init failed to create AlphaBlendState. HRESULT: 0x%08X", hr);
            return;
        }

        (void)spec;
    }

    void Renderer::Shutdown() {
        delete s_Data;
        s_Data = nullptr;
    }

    void Renderer::BeginScene(const Math::Mat4& viewProjection) {
        s_Data->ViewProjection = viewProjection;
        s_Data->CommandQueue.clear();

        s_Data->Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        SceneConstants sceneData = {};
        sceneData.ViewProjection = s_Data->ViewProjection;
        sceneData.LightDirection[0] = s_Data->Light.Direction.x;
        sceneData.LightDirection[1] = s_Data->Light.Direction.y;
        sceneData.LightDirection[2] = s_Data->Light.Direction.z;
        sceneData.LightColor[0] = s_Data->Light.Color[0];
        sceneData.LightColor[1] = s_Data->Light.Color[1];
        sceneData.LightColor[2] = s_Data->Light.Color[2];
        sceneData.LightIntensity = s_Data->Light.Itensity;
        sceneData.AmbientStrength = s_Data->Light.AmbientStrength;

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(s_Data->Context->Map(s_Data->SceneCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, &sceneData, sizeof(SceneConstants));
            s_Data->Context->Unmap(s_Data->SceneCB.Get(), 0);
        }
        s_Data->Context->VSSetConstantBuffers(0, 1, s_Data->SceneCB.GetAddressOf());
        s_Data->Context->PSSetConstantBuffers(0, 1, s_Data->SceneCB.GetAddressOf());
    }

    void Renderer::Submit(const Entity& entity) {
        if (!entity.Mesh || !entity.Material) return; // nothing to draw, skip safely
        
        RenderCommand cmd;
        cmd.VertexBuffer = entity.Mesh->GetVertexBuffer();
        cmd.IndexBuffer = entity.Mesh->GetIndexBuffer();
        cmd.IndexCount = entity.Mesh->GetIndexCount();
        cmd.Stride = entity.Mesh->GetStride(); 
        cmd.VS = entity.Material->GetShader()->GetVS();
        cmd.PS = entity.Material->GetShader()->GetPS();
        cmd.Layout = entity.Material->GetShader()->GetLayout();
        
        auto texture = entity.Material->GetTexture();
        if (!texture) texture = s_Data->WhiteTexture;

        cmd.SRV = texture->GetSRV();
        cmd.Sampler = texture->GetSampler();

        cmd.Transform = entity.GetTransform();

        s_Data->CommandQueue.push_back(cmd);
    }

    void Renderer::Flush() {
        auto* ctx = s_Data->Context;

        const float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
        ctx->OMSetBlendState(s_Data->AlphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
        
        std::sort(s_Data->CommandQueue.begin(), s_Data->CommandQueue.end(),
            [](const RenderCommand& a, const RenderCommand& b) {
                return a.VS < b.VS;
            });

        ID3D11VertexShader* lastVS = nullptr;
        ID3D11PixelShader* lastPS = nullptr;
        ID3D11InputLayout* lastLayout = nullptr;

        for (auto& cmd : s_Data->CommandQueue) {
            if (cmd.VS != lastVS) { ctx->VSSetShader(cmd.VS, nullptr, 0); lastVS = cmd.VS; }
            if (cmd.PS != lastPS) { ctx->PSSetShader(cmd.PS, nullptr, 0); lastPS = cmd.PS; }
            if (cmd.Layout != lastLayout) { ctx->IASetInputLayout(cmd.Layout); lastLayout = cmd.Layout; }

            UINT stride = cmd.Stride, offset = 0;
            ctx->IASetVertexBuffers(0, 1, &cmd.VertexBuffer, &stride, &offset);
            ctx->IASetIndexBuffer(cmd.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

            // Bind texture + sampler if present (no per-frame "last" caching yet — fine for now,
            // worth adding if you have many draws sharing the same texture)
            ctx->PSSetShaderResources(0, 1, &cmd.SRV);
            ctx->PSSetSamplers(0, 1, &cmd.Sampler);
             
            EntityConstants entityData{};
            entityData.World = cmd.Transform;

            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(ctx->Map(s_Data->EntityCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, &entityData, sizeof(EntityConstants));
                ctx->Unmap(s_Data->EntityCB.Get(), 0);
            }
            ctx->VSSetConstantBuffers(1, 1, s_Data->EntityCB.GetAddressOf());

            ctx->DrawIndexed(cmd.IndexCount, 0, 0);
        }

        s_Data->CommandQueue.clear();

        // Restore default (no blending) so ImGui and other passes are unaffected.
        ctx->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    }

    void Renderer::EndScene() {
        Flush();
    }
}