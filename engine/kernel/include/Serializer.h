#pragma once

#include "Entity.h"
#include "Ref.h"
#include "Scene.h"
#include <memory>
#include <mutex>
#include <string>

namespace arch::core::kernel {

class Serializer {
public:
  explicit Serializer(std::unique_ptr<Scene> scene)
      : _activeScene{std::move(scene)} {}
  ~Serializer() = default;

  Serializer(const Serializer &) = delete;
  Serializer &operator=(const Serializer &) = delete;

  Ref<Scene> GetActiveScene() { return _activeScene.get(); }
  bool HasActiveScene() const { return _activeScene != nullptr; }

  // DSL Text Serialization
  void SerializeEntity(std::ostream &out, const Entity &entity);
  void Serialize(const std::string &filepath);
  bool Deserialize(const std::string &filepath);

  // Binary Runtime Serialization
  void SerializeRuntime(const std::string &filepath);
  bool DeserializeRuntime(const std::string &filepath);

private:
  std::unique_ptr<Scene> _activeScene;
  mutable std::mutex _mutex;
};
} // namespace arch::core::kernel