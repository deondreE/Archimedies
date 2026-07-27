#pragma once
#include "archpch.h"
#include "Mesh.h"
#include "Material.h"

namespace Engine {
    class UUID {
    public:
        UUID() : _Value(s_UniformDistribution(s_Engine)) {}
        operator uint64_t() const { return _Value; }
    private:
        uint64_t _Value;
        static inline std::random_device s_RandomDevice;
        static inline std::mt19937_64 s_Engine{ s_RandomDevice() };
        static inline std::uniform_int_distribution<uint64_t> s_UniformDistribution;
    };

    struct Entity {
        UUID ID;
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