#pragma once

#include "ITexture.h"
#include "Maths.h"
#include "Mesh.h"
#include "UUID.h"
#include "types.h"
#include "Ref.h"
#include "IMaterial.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace arch::core::kernel {

using namespace arch::core::math;

inline u32 GetUniqueComponentID() {
  static u32 lastID = 0;
  return lastID++;
}

template <typename T> inline u32 GetComponentTypeID() {
  static u32 typeID = GetUniqueComponentID();
  return typeID;
}

/** Refers to a derived structure that something requires, makes scripting
 * simpler overall. */
struct Component {
  UUID id;
  UUID ent_id;
  u32 type_id;

  Component(u32 typeID) : type_id(typeID) {}
  virtual ~Component() = default;
};

struct TransformComponent : public Component {
  Vector3 pos{0.0f};
  Vector3 rotation{0.0f};
  Vector3 scale{1.0f};

  TransformComponent() : Component(GetComponentTypeID<TransformComponent>()) {}
};

struct MaterialComponent : public Component {
  Ref<renderer::IMaterial> material;

  MaterialComponent() : Component(GetComponentTypeID<MaterialComponent>()) {}
};

/** The renderer will look for a Mesh, and if no Material Component is found
 * then it will add a default material. */
struct MeshComponent : public Component {
  Mesh mesh;
  std::string mesh_path;

  MeshComponent() : Component(GetComponentTypeID<MeshComponent>()) {}
};

/** Refers to everthing that requires a sprite.  */
struct SpriteComponent : public Component {
  Ref<renderer::ITexture> texture;

  SpriteComponent() : Component(GetComponentTypeID<SpriteComponent>()) {}
};

struct Entity {
  UUID id;
  std::string name;
  Mesh mesh;

  std::vector<std::unique_ptr<Component>> components;

  Entity(const std::string &name) : name{name} {}

  Entity(const Entity &) = delete;
  Entity &operator=(const Entity &) = delete;
  Entity(Entity &&) noexcept = default;
  Entity &operator=(Entity &&) noexcept = default;

  template <typename T, typename... Args> T &AddComponent(Args &&...args) {
    auto comp = std::make_unique<T>(std::forward<Args>(args)...);
    comp->ent_id = this->id;
    T &ref = *comp;
    components.push_back(std::move(comp));
    return ref;
  }

  template <typename T> T *GetComponent() const {
    for (auto &comp : components) {
      if (T *target = dynamic_cast<T *>(comp.get())) {
        return target;
      }
    }
    return nullptr;
  }

  template <typename T> bool HasComponent() const {
    return GetComponent<T>() != nullptr;
  }

  template <typename T> void RemoveComponent() {
    u32 targetID = GetComponentTypeID<T>();
    components.erase(std::remove_if(components.begin(), components.end(),
                                    [targetID](const auto &comp) {
                                      return comp->type_id == targetID;
                                    }),
                     components.end());
  }

  // Remove by pointer
  void RemoveComponent(const Component *target) {
    components.erase(std::remove_if(components.begin(), components.end(),
                                    [target](const auto &comp) {
                                      return comp.get() == target;
                                    }),
                     components.end());
  }
};
} // namespace arch::core::kernel