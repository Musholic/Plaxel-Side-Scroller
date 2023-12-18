#include "../test_utils.h"

#include <gtest/gtest.h>

using namespace plaxel::test;

TEST(RendererTest, SimpleDrawing) {
  // Arrange
  const auto testName = "simple_drawing_test";

  // Act
  drawAndSaveScreenshot(testName);

  // Assert
  EXPECT_EQ(compareImages(testName), 0);
}

TEST(RendererTest, OneCube) {
  // Arrange
  TestRenderer renderer;
  renderer.setVoxelTreeNode({1});

  // Act
  const auto data = drawAndGetTriangles(renderer);

  // Assert
  const std::string expected =
      "{1;0;0(0;1),0;0;0(1;1),0;1;0(1;0)}, {0;1;0(1;0),1;1;0(0;0),1;0;0(0;1)}, "
      "{0;0;1(0;1),1;0;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),0;1;1(0;0),0;0;1(0;1)}, "
      "{0;0;0(0;1),0;0;1(1;1),0;1;1(1;0)}, {0;1;1(1;0),0;1;0(0;0),0;0;0(0;1)}, "
      "{1;0;1(0;1),1;0;0(1;1),1;1;0(1;0)}, {1;1;0(1;0),1;1;1(0;0),1;0;1(0;1)}, "
      "{0;0;1(0;1),0;0;0(1;1),1;0;0(1;0)}, {1;0;0(1;0),1;0;1(0;0),0;0;1(0;1)}, "
      "{0;1;0(0;1),0;1;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),1;1;0(0;0),0;1;0(0;1)}";
  EXPECT_EQ(toString(data), expected);
}

TEST(RendererTest, TwoCubes) {
  // Arrange
  TestRenderer renderer;
  renderer.setVoxelTreeNode({1, 1});

  // Act
  const auto data = drawAndGetTriangles(renderer);

  // Assert
  const std::string expected =
      "{1;0;0(0;1),0;0;0(1;1),0;1;0(1;0)}, {0;1;0(1;0),1;1;0(0;0),1;0;0(0;1)}, "
      "{0;0;1(0;1),1;0;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),0;1;1(0;0),0;0;1(0;1)}, "
      "{0;0;0(0;1),0;0;1(1;1),0;1;1(1;0)}, {0;1;1(1;0),0;1;0(0;0),0;0;0(0;1)}, "
      "{0;0;1(0;1),0;0;0(1;1),1;0;0(1;0)}, {1;0;0(1;0),1;0;1(0;0),0;0;1(0;1)}, "
      "{0;1;0(0;1),0;1;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),1;1;0(0;0),0;1;0(0;1)}, "

      "{2;0;0(0;1),1;0;0(1;1),1;1;0(1;0)}, {1;1;0(1;0),2;1;0(0;0),2;0;0(0;1)}, "
      "{1;0;1(0;1),2;0;1(1;1),2;1;1(1;0)}, {2;1;1(1;0),1;1;1(0;0),1;0;1(0;1)}, "
      "{2;0;1(0;1),2;0;0(1;1),2;1;0(1;0)}, {2;1;0(1;0),2;1;1(0;0),2;0;1(0;1)}, "
      "{1;0;1(0;1),1;0;0(1;1),2;0;0(1;0)}, {2;0;0(1;0),2;0;1(0;0),1;0;1(0;1)}, "
      "{1;1;0(0;1),1;1;1(1;1),2;1;1(1;0)}, {2;1;1(1;0),2;1;0(0;0),1;1;0(0;1)}";
  EXPECT_EQ(toString(data), expected);
}
