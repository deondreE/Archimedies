#include "Maths.h"
#include <gtest/gtest.h>
#include <sstream>

namespace arch::core::math::test {

TEST(MathTest, Vector3Operations) {
  Vector3 a(1.0f, 2.0f, 3.0f);
  Vector3 b(4.0f, 5.0f, 6.0f);

  Vector3 c = a + b;

  EXPECT_FLOAT_EQ(c.x, 5.0f);
  EXPECT_FLOAT_EQ(c.y, 7.0f);
  EXPECT_FLOAT_EQ(c.z, 9.0f);
}

TEST(MathTest, Vector3Serialization) {
  Vector3 original(1.5f, 2.5f, 3.5f);
  std::stringstream ss;

  // Serialize
  ss << original;

  // Deserialize
  Vector3 loaded;
  ss >> loaded;

  EXPECT_FLOAT_EQ(original.x, loaded.x);
  EXPECT_FLOAT_EQ(original.y, loaded.y);
  EXPECT_FLOAT_EQ(original.z, loaded.z);
}

TEST(MathTest, Matrix4Identity) {
  Matrix4 identity = Matrix4::identity();

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (i == j)
        EXPECT_FLOAT_EQ(identity(i, j), 1.0f);
      else
        EXPECT_FLOAT_EQ(identity(i, j), 0.0f);
    }
  }
}

TEST(MathTest, Matrix4Multiplication) {
  Matrix4 a(2.0f); // Diagonal matrix with 2s
  Matrix4 b = Matrix4::identity();

  Matrix4 c = a * b;

  // Multiplying by identity should return the original diagonal matrix
  EXPECT_FLOAT_EQ(c(0, 0), 2.0f);
  EXPECT_FLOAT_EQ(c(1, 1), 2.0f);
  EXPECT_FLOAT_EQ(c(3, 3), 2.0f);
}

TEST(MathTest, Matrix4Serialization) {
  Matrix4 original = Matrix4::identity();
  original(0, 3) = 10.0f; // Set a translation value

  std::stringstream ss;
  ss << original;

  Matrix4 loaded;
  ss >> loaded;

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_FLOAT_EQ(original(i, j), loaded(i, j));
    }
  }
}

TEST(MathTest, GraphicsDataConsistency) {
  Vector3 v(1.0f, 2.0f, 3.0f);
  const float *raw = v.data();

  // Ensure data() points to the first element and is contiguous
  EXPECT_EQ(raw[0], 1.0f);
  EXPECT_EQ(raw[1], 2.0f);
  EXPECT_EQ(raw[2], 3.0f);
}

} // namespace arch::core::math::test