#include "Entity.h"
#include <gtest/gtest.h>

namespace arch::core::kernel::test {

class EntityTest : public ::testing::Test {
protected:
  void SetUp() override {}
};

TEST_F(EntityTest, EntityCreation) {
  Entity entity("TestEntity");
  EXPECT_EQ(entity.name, "TestEntity");
  EXPECT_NE(entity.id.GetLow(), 0);
}

TEST_F(EntityTest, AddAndGetComponent) {
  Entity entity("Player");

  auto &transform = entity.AddComponent<TransformComponent>();
  transform.pos = {1.0f, 2.0f, 3.0};

  TransformComponent *retrieved = entity.GetComponent<TransformComponent>();

  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->pos.x, 1.0f);
  EXPECT_EQ(retrieved->ent_id, entity.id);
}

TEST_F(EntityTest, HasComponentCheck) {
  Entity entity("Check Test");
  EXPECT_FALSE(entity.HasComponent<TransformComponent>());
  entity.AddComponent<TransformComponent>();

  EXPECT_TRUE(entity.HasComponent<TransformComponent>());
}

TEST_F(EntityTest, UniqueUUIDs) {
  Entity e1("E1");
  Entity e2("E2");

  EXPECT_NE(e1.id, e2.id);
}

TEST_F(EntityTest, StaticTypeIds) {
  u32 id1 = GetComponentTypeID<TransformComponent>();

  struct MockComponent : public Component {
    MockComponent() : Component(GetComponentTypeID<MockComponent>()) {}
  };

  u32 id2 = GetComponentTypeID<MockComponent>();

  EXPECT_NE(id1, id2);
  EXPECT_EQ(id1, GetComponentTypeID<TransformComponent>());
}

TEST_F(EntityTest, RemoveComponent) {
  Entity entity("Eraser");
  auto &transform = entity.AddComponent<TransformComponent>();
  UUID componentID = transform.id;

  ASSERT_TRUE(entity.HasComponent<TransformComponent>());

  entity.RemoveComponent<TransformComponent>();

  EXPECT_FALSE(entity.HasComponent<TransformComponent>());
  EXPECT_EQ(entity.GetComponent<TransformComponent>(), nullptr);
}

TEST_F(EntityTest, MoveSemantics) {
  Entity entity("Original");
  entity.AddComponent<TransformComponent>();

  // Move the entity to a new variable
  Entity movedEntity = std::move(entity);

  EXPECT_EQ(movedEntity.name, "Original");
  EXPECT_TRUE(movedEntity.HasComponent<TransformComponent>());

  // Note: 'entity' is now in a moved-from state
}

} // namespace arch::core::kernel::test