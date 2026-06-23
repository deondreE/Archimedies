#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>

#include "Serializer.h"

namespace arch::core::kernel::test {

class SerializerTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto scene = std::make_unique<Scene>();
    _serializer = std::make_unique<Serializer>(std::move(scene));
    _testFile = "test_scene.arch";
    _testBinaryFile = "test_scene.bin";
  }

  void TearDown() override {
    if (std::filesystem::exists(_testFile)) {
      std::filesystem::remove(_testFile);
    }
    if (std::filesystem::exists(_testBinaryFile)) {
      std::filesystem::remove(_testBinaryFile);
    }
  }

  std::unique_ptr<Serializer> _serializer;
  std::string _testFile;
  std::string _testBinaryFile;
};

TEST_F(SerializerTest, TextSerializationRoundTrip) {
  {
    auto scene = _serializer->GetActiveScene();
    Entity &player = scene->entities.emplace_back("Player");
    auto &t = player.AddComponent<TransformComponent>();
    t.pos = {10.0f, 5.0f, -2.0f};
  }

  _serializer->Serialize(_testFile);
  EXPECT_TRUE(std::filesystem::exists(_testFile));

  auto newScene = std::make_unique<Scene>();
  Serializer deserializer(std::move(newScene));
  bool success = deserializer.Deserialize(_testFile);

  ASSERT_TRUE(success);
  auto loadedScene = deserializer.GetActiveScene();
  ASSERT_EQ(loadedScene->entities.size(), 1);

  Entity &ent = loadedScene->entities[0];
  EXPECT_EQ(ent.name, "Player");

  auto *t = ent.GetComponent<TransformComponent>();
  ASSERT_NE(t, nullptr);
  EXPECT_FLOAT_EQ(t->pos.x, 10.0f);
  EXPECT_FLOAT_EQ(t->pos.y, 5.0f);
  EXPECT_FLOAT_EQ(t->pos.z, -2.0f);
}

TEST_F(SerializerTest, MeshComponentSerialization) {
  const std::string testPath = "assets/models/viking_room.obj";
  {
    auto scene = _serializer->GetActiveScene();
    Entity &ent = scene->entities.emplace_back("ModelEntity");
    auto &mesh = ent.AddComponent<MeshComponent>();
    mesh.mesh_path = testPath;
  }

  _serializer->Serialize(_testFile);

  auto newScene = std::make_unique<Scene>();
  Serializer deserializer(std::move(newScene));
  ASSERT_TRUE(deserializer.Deserialize(_testFile));

  auto loadedScene = deserializer.GetActiveScene();
  Entity &ent = loadedScene->entities[0];
  auto *mesh = ent.GetComponent<MeshComponent>();

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->mesh_path, testPath);
}

TEST_F(SerializerTest, MultiComponentEntity) {
  {
    auto scene = _serializer->GetActiveScene();
    Entity &ent = scene->entities.emplace_back("ComplexEntity");

    auto &t = ent.AddComponent<TransformComponent>();
    t.pos = {5.0f, 5.0f, 5.0f};

    auto &m = ent.AddComponent<MeshComponent>();
    m.mesh_path = "cube.fbx";
  }

  _serializer->Serialize(_testFile);

  auto newScene = std::make_unique<Scene>();
  Serializer deserializer(std::move(newScene));
  deserializer.Deserialize(_testFile);

  Entity &ent = deserializer.GetActiveScene()->entities[0];

  EXPECT_TRUE(ent.HasComponent<TransformComponent>());
  EXPECT_TRUE(ent.HasComponent<MeshComponent>());

  EXPECT_FLOAT_EQ(ent.GetComponent<TransformComponent>()->pos.x, 5.0f);
  EXPECT_EQ(ent.GetComponent<MeshComponent>()->mesh_path, "cube.fbx");
}

TEST_F(SerializerTest, QuotedStringSpaces) {
  const std::string nameWithSpaces = "Player Character Instance";
  const std::string pathWithSpaces = "assets/my models/test file.obj";
  {
    auto scene = _serializer->GetActiveScene();
    Entity &ent = scene->entities.emplace_back(nameWithSpaces);
    ent.AddComponent<MeshComponent>().mesh_path = pathWithSpaces;
  }

  _serializer->Serialize(_testFile);

  auto newScene = std::make_unique<Scene>();
  Serializer deserializer(std::move(newScene));
  ASSERT_TRUE(deserializer.Deserialize(_testFile));

  Entity &ent = deserializer.GetActiveScene()->entities[0];
  EXPECT_EQ(ent.name, nameWithSpaces);
  EXPECT_EQ(ent.GetComponent<MeshComponent>()->mesh_path, pathWithSpaces);
}

TEST_F(SerializerTest, BinaryRuntimeSerialization) {
  {
    auto scene = _serializer->GetActiveScene();
    Entity &npc = scene->entities.emplace_back("NPC");
    npc.AddComponent<TransformComponent>().pos = {1.0f, 1.0f, 1.0f};
  }

  _serializer->SerializeRuntime(_testBinaryFile);
  EXPECT_TRUE(std::filesystem::exists(_testBinaryFile));

  auto size = std::filesystem::file_size(_testBinaryFile);
  EXPECT_GT(size, 0);

  std::ifstream file(_testBinaryFile, std::ios::binary);
  u32 magic = 0;
  file.read(reinterpret_cast<char *>(&magic), sizeof(magic));
  EXPECT_EQ(magic, 0x41524348);
}

TEST_F(SerializerTest, EmptySceneHandling) {
  _serializer->Serialize(_testFile);

  auto emptyScene = std::make_unique<Scene>();
  Serializer deserializer(std::move(emptyScene));

  EXPECT_TRUE(deserializer.Deserialize(_testFile));
  EXPECT_EQ(deserializer.GetActiveScene()->entities.size(), 0);
}

TEST_F(SerializerTest, MultipleEntities) {
  {
    auto scene = _serializer->GetActiveScene();
    scene->entities.emplace_back("E1");
    scene->entities.emplace_back("E2");
    scene->entities.emplace_back("E3");
  }

  _serializer->Serialize(_testFile);

  auto newScene = std::make_unique<Scene>();
  Serializer deserializer(std::move(newScene));
  deserializer.Deserialize(_testFile);

  auto loadedScene = deserializer.GetActiveScene();
  EXPECT_EQ(loadedScene->entities.size(), 3);
  EXPECT_EQ(loadedScene->entities[0].name, "E1");
  EXPECT_EQ(loadedScene->entities[2].name, "E3");
}

} // namespace arch::core::kernel::test