#pragma once
#include "archpch.h"

namespace Engine {

	struct Vertex {
		Math::Vec3 Position;
		float Color[4];
		float UV[2];
		float Normal[3];
	};

	class Mesh {
	public:
		static std::shared_ptr<Mesh> Create(ID3D11Device* device,
			const std::vector<Vertex>& vertices,
			const std::vector<uint32_t>& indices,
			const std::string& path = "");
		static std::shared_ptr<Mesh> LoadFromFile(ID3D11Device* device, const std::string& filepath);

		ID3D11Buffer* GetVertexBuffer() const { return _VertexBuffer.Get(); }
		ID3D11Buffer* GetIndexBuffer() const { return _IndexBuffer.Get(); }
		uint32_t GetIndexCount() const { return _IndexCount; }
		uint32_t GetStride() const { return sizeof(Vertex); }
		const std::string& GetPath() const { return _FilePath; }
	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> _VertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _IndexBuffer;
		uint32_t _IndexCount = 0;
		std::string _FilePath;
	};
}
