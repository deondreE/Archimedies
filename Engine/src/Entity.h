#pragma once
#include "archpch.h"
#include "Mesh.h"
#include "Material.h"

namespace Engine {

    struct Entity {
        std::string Name;
        Math::Vec3 Position;

        std::shared_ptr<Mesh> Mesh;
        std::shared_ptr<Material> Material;

        Math::Mat4 GetTransform() const {
            return Math::Mat4::Translation(Position);
        }
    };
}