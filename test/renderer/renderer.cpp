#include "../test_utils.h"

#include <gtest/gtest.h>

using namespace plaxel::test;

TEST(RendererTest, Test) {
  // Arrange
  const auto testName = "simple_drawing_test";

  // Act
  drawAndSaveScreenshot(testName);

  // Assert
  EXPECT_EQ(compareImages(testName), 0);
}
