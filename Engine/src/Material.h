#pragma once
#include "Shader.h"
#include "Texture.h"

namespace Engine {

    class Material {
    public:
        Material(std::shared_ptr<Shader> shader, std::shared_ptr<Texture2D> texture = nullptr) : _Shader(shader), _Texture(texture) {}

        std::shared_ptr<Shader> GetShader() const { return _Shader; }
        std::shared_ptr<Texture2D> GetTexture() const { return _Texture; }
        void SetTexture(std::shared_ptr<Texture2D> texture) { _Texture = texture; }

    private:
        std::shared_ptr<Shader> _Shader;
        std::shared_ptr<Texture2D> _Texture;
        // Later: textures, per-material constant data (color tint, roughness, etc.)
    };
}