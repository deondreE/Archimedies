#pragma once
#include "archpch.h"
#include "Mesh.h"
#include "Material.h"

namespace Engine {

    struct Entity {
        std::string Name;
        Math::Vec3 Position = { 0.0f, 0.0f, 0.0f };
        Math::Vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        Math::Vec3 Scale = { 1.0f, 1.0f, 1.0f };

        std::shared_ptr<Mesh> Mesh;
        std::shared_ptr<Material> Material;

        // (S * R * T)
        Math::Mat4 GetTransform() const {
            return
                Math::Mat4::Scale(Scale) *
                Math::Mat4::RotationEuler(Rotation) *
                Math::Mat4::Translation(Position);
        }
    };
}