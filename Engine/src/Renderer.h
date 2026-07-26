#pragma once
#include "archpch.h"
#include "Scene.h"
#include "ShaderLibrary.h"
#include "Light.h";

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
		Math::Mat4 Transform; // World Matrix
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
		static DirectionalLight& GetLight() { return s_Data->Light; }
	private:
		static void Flush();

		struct SceneConstants {
			Math::Mat4 ViewProjection;
			float LightDirection[3];
			float _Pad0;
			float LightColor[3];
			float LightIntensity;
			float AmbientStrength;
			float _Pad1;
		};
		static_assert(sizeof(SceneConstants) % 16 == 0, "SceneConstants must be 16-byte aligned"); 

		struct EntityConstants {
			Math::Mat4 World;
		};
		static_assert(sizeof(EntityConstants) % 16 == 0, "EntityConstants must be 16-byte aligned");

		struct RendererData {
			ID3D11Device* Device;
			ID3D11DeviceContext* Context;

			Microsoft::WRL::ComPtr<ID3D11Buffer> ConstantBuffer;

			// Reserved for splitting per-frame (view/proj/lights) vs
			// per-draw (world transform) constant data later.
			Microsoft::WRL::ComPtr<ID3D11Buffer> SceneCB;
			Microsoft::WRL::ComPtr<ID3D11Buffer> EntityCB;

			Math::Mat4 ViewProjection;
			DirectionalLight Light;

			std::vector<RenderCommand> CommandQueue;
			ShaderLibrary Shaders;
			std::shared_ptr<Texture2D> WhiteTexture;
		};

		static RendererData* s_Data;
	};
}