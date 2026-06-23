#include "Serializer.h"
#include "Entity.h"
#include "IMaterial.h"
#include "types.h"
#include <fstream>
#include <iomanip>
#include <mutex>
#include <ostream>

namespace arch::core::kernel {

void Serializer::SerializeEntity(std::ostream &out, const Entity &entity) {
  out << "ENTITY " << std::quoted(entity.name) << "\n";
  out << "  UUID " << entity.id.GetHigh() << " " << entity.id.GetLow() << "\n";

  for (const auto &comp : entity.components) {
    out << "  COMPONENT " << comp->type_id << "\n";
    if (comp->type_id == GetComponentTypeID<TransformComponent>()) {
      auto *t = static_cast<TransformComponent *>(comp.get());
      out << "    POS " << t->pos << "\n";
      out << "    ROT " << t->rotation << "\n";
      out << "    SCA " << t->scale << "\n";
    } else if (comp->type_id == GetComponentTypeID<MeshComponent>()) {
      auto *m = static_cast<MeshComponent *>(comp.get());
      out << "   PATH " << std::quoted(m->mesh_path) << "\n";
    } else if (comp->type_id == GetComponentTypeID<MaterialComponent>()) {
      auto *mat = static_cast<MaterialComponent *>(comp.get());
      std::string matName = mat->material ? mat->material->GetName() : "None";
      out << "    MATNAME " << std::quoted(matName) << "\n";
    } else if (comp->type_id == GetComponentTypeID<SpriteComponent>()) {
      auto *s = static_cast<SpriteComponent *>(comp.get());
      out << "    TEXTURE_REF \"DefaultSprite\"\n";
    }

    out << "  END_COMP\n";
  }
  out << "END_ENTITY\n";
}

void Serializer::Serialize(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_activeScene)
    return;

  std::ofstream file(filepath);
  if (!file.is_open())
    return;

  file << "ARCH_SCENE_V1\n";
  for (const auto &entity : _activeScene->entities) {
    SerializeEntity(file, entity);
  }
}

bool Serializer::Deserialize(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(_mutex);
  std::ifstream file(filepath);
  if (!file.is_open())
    return false;

  std::string line;
  file >> line; // Header: ARCH_SCENE_V1

  std::string token;
  while (file >> token) {
    if (token == "ENTITY") {
      // Properly handle quoted name
      std::string name;
      file >> std::quoted(name); // Use std::quoted to strip " "

      Entity &ent = _activeScene->entities.emplace_back(name);

      while (file >> token && token != "END_ENTITY") {
        if (token == "UUID") {
          u64 high, low;
          file >> high >> low;
          ent.id = UUID(low, high);
        } else if (token == "COMPONENT") {
          u32 typeID;
          file >> typeID;
          if (typeID == GetComponentTypeID<TransformComponent>()) {
            auto &t = ent.AddComponent<TransformComponent>();
            while (file >> token && token != "END_COMP") {
              if (token == "POS")
                file >> t.pos;
              if (token == "ROT")
                file >> t.rotation;
              if (token == "SCA")
                file >> t.scale;
            }
          } else if (typeID == GetComponentTypeID<MeshComponent>()) {
            auto &m = ent.AddComponent<MeshComponent>();
            while (file >> token && token != "END_COMP") {
              if (token == "PATH")
                file >> std::quoted(m.mesh_path);
            }
          } else if (typeID == GetComponentTypeID<MaterialComponent>()) {
            auto &mat = ent.AddComponent<MaterialComponent>();
            while (file >> token && token != "END_COMP") {
              std::string matName;
              if (token == "MATNAME")
                file >> std::quoted(matName);
              // @Todo: mat.material = AssetManager::GetMaterial(matName);
            }
          }
        }
      }
    }
  }
  return true;
}

void Serializer::SerializeRuntime(const std::string &filepath) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_activeScene)
    return;

  std::ofstream file(filepath, std::ios::binary);

  // Binary Tag Header
  u32 magic = 0x41524348; // "ARCH"
  file.write(reinterpret_cast<char *>(&magic), sizeof(magic));

  for (const auto &entity : _activeScene->entities) {
    // Write UUID raw
    u64 high = entity.id.GetHigh();
    u64 low = entity.id.GetLow();
    file.write(reinterpret_cast<char *>(&high), sizeof(high));
    file.write(reinterpret_cast<char *>(&low), sizeof(low));

    u32 compCount = (u32)entity.components.size();
    file.write(reinterpret_cast<char *>(&compCount), sizeof(compCount));

    for (const auto &comp : entity.components) {
      file.write(reinterpret_cast<char *>(&comp->type_id),
                 sizeof(comp->type_id));
      if (comp->type_id == GetComponentTypeID<TransformComponent>()) {
        auto *t = static_cast<TransformComponent *>(comp.get());
        file.write(reinterpret_cast<char *>(&t->pos), sizeof(Vector3));
        file.write(reinterpret_cast<char *>(&t->rotation), sizeof(Vector3));
        file.write(reinterpret_cast<char *>(&t->scale), sizeof(Vector3));
      } else if (comp->type_id == GetComponentTypeID<MeshComponent>()) {
        auto *m = static_cast<MeshComponent *>(comp.get());
        u32 pathLen = (u32)m->mesh_path.length();
        file.write(reinterpret_cast<char *>(&pathLen), sizeof(pathLen));
        file.write(m->mesh_path.c_str(), pathLen);
      }
    }
  }
}

bool DeserializeRuntime(const std::string &filepath) { return A_UNIMPLEMENTED; }
} // namespace arch::core::kernel