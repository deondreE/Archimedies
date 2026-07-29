#pragma once
#include "Shader.h"
#include "Texture.h"

#define SOLUTION_DIR L"C:/Users/deond/code/Archimedies"

#include <filesystem>
namespace fs = std::filesystem;

namespace Engine {

    class Material {
    public:
        Material(std::shared_ptr<Shader> shader, std::shared_ptr<Texture2D> texture = nullptr) : _Shader(shader), _Texture(texture) {}

        std::shared_ptr<Shader> GetShader() const { return _Shader; }
        std::shared_ptr<Texture2D> GetTexture() const { return _Texture; }
        void SetTexture(std::shared_ptr<Texture2D> texture) { _Texture = texture; }

        static void SetDefaultShaderPath(const std::wstring& path) {
            s_DefaultShaderPath = path;
            s_DefaultMaterial.reset(); // force rebuild with new path if already created
        }

        static std::wstring GetDefaultShaderPath() {
            return s_DefaultShaderPath;
        }

        static std::shared_ptr<Material> GetDefault(ID3D11Device* device) {
            if (!s_DefaultMaterial) {
                auto shader = Shader::Create(device, s_DefaultShaderPath);
                auto texture = Texture2D::GetDefaultTexture(device);
                s_DefaultMaterial = std::make_shared<Material>(shader, texture);
            }
            return s_DefaultMaterial;
        }


    private:

        static inline std::wstring s_DefaultShaderPath =
            (fs::path(SOLUTION_DIR) / "Engine/src/Assets/Basic.hlsl").wstring();
        static std::shared_ptr<Material> s_DefaultMaterial;

        std::shared_ptr<Shader> _Shader;
        std::shared_ptr<Texture2D> _Texture;
        // Later: textures, per-material constant data (color tint, roughness, etc.)
    };
}