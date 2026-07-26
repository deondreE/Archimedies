#pragma once
#include "archpch.h"
#include "Scene.h"
#include "ShaderLibrary.h"

namespace Engine {

	struct RenderCommand {
		ID3D11Buffer* VertexBuffer;
		ID3D11Buffer* IndexBuffer;
		uint32_t IndexCount;
		uint32_t Stride;
		ID3D11VertexShader* VS;
		ID3D11PixelShader* PS;
		ID3D11InputLayout* Layout;
		ID3D11ShaderResourceView* SRV;
		ID3D11SamplerState* Sampler;
		Math::Mat4 Transform;
	};

	struct ApplicationSpecification;

	class Renderer {
	public:
		static void Init(ID3D11Device* device, ID3D11DeviceContext* context, const ApplicationSpecification& spec);
		static void Shutdown();

		static void BeginScene(const Math::Mat4& viewProjection);
		static void EndScene();

		static void Submit(const Entity& entity);

		static ShaderLibrary& GetShaderLibrary() { return s_Data->Shaders; }
	private:
		static void Flush();

		struct RendererData {
			ID3D11Device* Device;
			ID3D11DeviceContext* Context;

			Microsoft::WRL::ComPtr<ID3D11Buffer> ConstantBuffer;

			// Reserved for splitting per-frame (view/proj/lights) vs
			// per-draw (world transform) constant data later.
			Microsoft::WRL::ComPtr<ID3D11Buffer> SceneCB;
			Microsoft::WRL::ComPtr<ID3D11Buffer> EntityCB;

			std::shared_ptr<Texture2D> WhiteTexture;

			Math::Mat4 ViewProjection;
			std::vector<RenderCommand> CommandQueue;
			ShaderLibrary Shaders;
		};

		static RendererData* s_Data;
	};
}