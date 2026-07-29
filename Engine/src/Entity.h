#pragma once
#include "archpch.h"
#include "Mesh.h"
#include "Material.h"
#include "Component.h"

namespace Engine {
   
    struct Entity {
        Util::UUID ID;
        std::string Name;
        Math::Vec3 Position = { 0.0f, 0.0f, 0.0f };
        Math::Vec3 Rotation = { 0.0f, 0.0f, 0.0f };
        Math::Vec3 Scale = { 1.0f, 1.0f, 1.0f };
            
        std::shared_ptr<Mesh> Mesh;
        std::shared_ptr<Material> Material;

        std::vector<std::unique_ptr<Component::Component>> Components;

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) 
        {
            static_assert(std::is_base_of_v<Component::Component, T>, "T must inherit from Component");
            
            auto newComponent = std::make_unique<T>(std::forward<Args>(args)...);
            T& ref = *newComponent;

            Components.push_back(std::move(newComponent));
            return ref;
        }

        template<typename T>
        T* GetComponent() {
            static_assert(std::is_base_of_v<Component::Component, T>, "T must inherit from Component");

            for (auto& comp : Components) {
                T* target = dynamic_cast<T*>(comp.get());
                if (target) return target;
            }

            return nullptr;
        }

        std::string GetID() const { return ID.ToString(); }

        // (S * R * T)
        Math::Mat4 GetTransform() const {
            return
                Math::Mat4::Scale(Scale) *
                Math::Mat4::RotationEuler(Rotation) *
                Math::Mat4::Translation(Position);
        }
    };
}