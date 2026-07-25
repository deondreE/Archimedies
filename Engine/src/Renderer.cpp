#include "archpch.h"
#include "Renderer.h"
#include "Application.h"

namespace Engine {
    struct RendererVertex {
        Math::Vec3 Position;
        float Color[4];
    };

	Renderer::RendererData* Renderer::s_Data = nullptr;

    void Renderer::Init(ID3D11Device* device, ID3D11DeviceContext* context, const ApplicationSpecification& spec) {
        s_Data = new RendererData();
        s_Data->Device = device;
        s_Data->Context = context;

        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbd.ByteWidth = sizeof(Math::Mat4);
        s_Data->Device->CreateBuffer(&cbd, nullptr, &s_Data->ConstantBuffer);

        s_Data->WhiteTexture = Texture2D::CreateSolidColor(device, 0xFFFFFFFF);
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
        cmd.Transform = entity.GetTransform();
        
        auto texture = entity.Material->GetTexture();
        if (!texture) texture = s_Data->WhiteTexture;
        cmd.SRV = texture->GetSRV();
        cmd.Sampler = texture->GetSampler();

        cmd.Transform = entity.GetTransform();

        s_Data->CommandQueue.push_back(cmd);
    }

    void Renderer::Flush() {
        auto* ctx = s_Data->Context;
        
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

            Math::Mat4 wvp = cmd.Transform * s_Data->ViewProjection;
            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(ctx->Map(s_Data->ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, &wvp, sizeof(Math::Mat4));
                ctx->Unmap(s_Data->ConstantBuffer.Get(), 0);
            }
            ctx->VSSetConstantBuffers(0, 1, s_Data->ConstantBuffer.GetAddressOf());

            ctx->DrawIndexed(cmd.IndexCount, 0, 0);
        }

        s_Data->CommandQueue.clear();
    }

    void Renderer::EndScene() {
        Flush();
    }
}