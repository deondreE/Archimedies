#pragma once
#include "archpch.h"

namespace Engine {

	struct Vertex {
		Math::Vec3 Position;
		float Color[4];
		float UV[2];
	};

	class Mesh {
	public:
		static std::shared_ptr<Mesh> Create(ID3D11Device* device,
			const std::vector<Vertex>& vertices,
			const std::vector<uint32_t>& indices);

		ID3D11Buffer* GetVertexBuffer() const { return _VertexBuffer.Get(); }
		ID3D11Buffer* GetIndexBuffer() const { return _IndexBuffer.Get(); }
		uint32_t GetIndexCount() const { return _IndexCount; }
		uint32_t GetStride() const { return sizeof(Vertex); }
	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> _VertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _IndexBuffer;
		uint32_t _IndexCount = 0;
	};
}
