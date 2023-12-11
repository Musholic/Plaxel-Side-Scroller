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

  // Act
  const auto data = drawAndGetTriangles();

  // Assert
  const std::string expected = "{-0.5;-0.5;0(1;0),0.5;-0.5;0(0;0),0.5;0.5;0(0;1)}, "
                               "{0.5;0.5;0(0;1),-0.5;0.5;0(1;1),-0.5;-0.5;0(1;0)}, "
                               "{-0.5;-0.5;-0.5(1;0),0.5;-0.5;-0.5(0;0),0.5;0.5;-0.5(0;1)}, "
                               "{0.5;0.5;-0.5(0;1),-0.5;0.5;-0.5(1;1),-0.5;-0.5;-0.5(1;0)}";
  EXPECT_EQ(toString(data), expected);
}
