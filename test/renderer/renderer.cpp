#include "../test_utils.h"

#include <boost/iostreams/filter/zlib.hpp>
#include <gtest/gtest.h>

using namespace plaxel;
using namespace plaxel::test;


TEST_F(RendererTest, SimpleDrawing) {
  // Arrange
  const auto testName = "simple_drawing_test";
  renderer.addBlock(0, 0, 0);
  // Move the cursor away since we don't want to include it in the screenshot
  renderer.moveCursor({500, 0, 0});

  // Act
  drawAndSaveScreenshot(renderer, testName);

  // Assert
  EXPECT_EQ(compareImages(testName), 0);
}

TEST_F(RendererTest, OneCube) {
  // Arrange
  renderer.addBlock(0, 0, 0);

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

TEST_F(RendererTest, TwoCubes) {
  // Arrange
  renderer.addBlock(0, 0, 0);
  renderer.addBlock(1, 0, 0);

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

TEST_F(RendererTest, OneBigCube) {
  // Arrange
  for (int x = 0; x < 2; ++x) {
    for (int y = 0; y < 2; ++y) {
      for (int z = 0; z < 2; ++z) {
        renderer.addBlock(x, y, z);
      }
    }
  }

  // Act
  const auto data = drawAndGetTriangles(renderer);

  // Assert
  const std::string expected =
      "{1;0;0(0;1),0;0;0(1;1),0;1;0(1;0)}, {0;1;0(1;0),1;1;0(0;0),1;0;0(0;1)}, "
      "{0;0;0(0;1),0;0;1(1;1),0;1;1(1;0)}, {0;1;1(1;0),0;1;0(0;0),0;0;0(0;1)}, "
      "{0;0;1(0;1),0;0;0(1;1),1;0;0(1;0)}, {1;0;0(1;0),1;0;1(0;0),0;0;1(0;1)}, "

      "{0;0;2(0;1),1;0;2(1;1),1;1;2(1;0)}, {1;1;2(1;0),0;1;2(0;0),0;0;2(0;1)}, "
      "{0;0;1(0;1),0;0;2(1;1),0;1;2(1;0)}, {0;1;2(1;0),0;1;1(0;0),0;0;1(0;1)}, "
      "{0;0;2(0;1),0;0;1(1;1),1;0;1(1;0)}, {1;0;1(1;0),1;0;2(0;0),0;0;2(0;1)}, "

      "{1;1;0(0;1),0;1;0(1;1),0;2;0(1;0)}, {0;2;0(1;0),1;2;0(0;0),1;1;0(0;1)}, "
      "{0;1;0(0;1),0;1;1(1;1),0;2;1(1;0)}, {0;2;1(1;0),0;2;0(0;0),0;1;0(0;1)}, "
      "{0;2;0(0;1),0;2;1(1;1),1;2;1(1;0)}, {1;2;1(1;0),1;2;0(0;0),0;2;0(0;1)}, "

      "{0;1;2(0;1),1;1;2(1;1),1;2;2(1;0)}, {1;2;2(1;0),0;2;2(0;0),0;1;2(0;1)}, "
      "{0;1;1(0;1),0;1;2(1;1),0;2;2(1;0)}, {0;2;2(1;0),0;2;1(0;0),0;1;1(0;1)}, "
      "{0;2;1(0;1),0;2;2(1;1),1;2;2(1;0)}, {1;2;2(1;0),1;2;1(0;0),0;2;1(0;1)}, "

      "{2;0;0(0;1),1;0;0(1;1),1;1;0(1;0)}, {1;1;0(1;0),2;1;0(0;0),2;0;0(0;1)}, "
      "{2;0;1(0;1),2;0;0(1;1),2;1;0(1;0)}, {2;1;0(1;0),2;1;1(0;0),2;0;1(0;1)}, "
      "{1;0;1(0;1),1;0;0(1;1),2;0;0(1;0)}, {2;0;0(1;0),2;0;1(0;0),1;0;1(0;1)}, "

      "{1;0;2(0;1),2;0;2(1;1),2;1;2(1;0)}, {2;1;2(1;0),1;1;2(0;0),1;0;2(0;1)}, "
      "{2;0;2(0;1),2;0;1(1;1),2;1;1(1;0)}, {2;1;1(1;0),2;1;2(0;0),2;0;2(0;1)}, "
      "{1;0;2(0;1),1;0;1(1;1),2;0;1(1;0)}, {2;0;1(1;0),2;0;2(0;0),1;0;2(0;1)}, "

      "{2;1;0(0;1),1;1;0(1;1),1;2;0(1;0)}, {1;2;0(1;0),2;2;0(0;0),2;1;0(0;1)}, "
      "{2;1;1(0;1),2;1;0(1;1),2;2;0(1;0)}, {2;2;0(1;0),2;2;1(0;0),2;1;1(0;1)}, "
      "{1;2;0(0;1),1;2;1(1;1),2;2;1(1;0)}, {2;2;1(1;0),2;2;0(0;0),1;2;0(0;1)}, "

      "{1;1;2(0;1),2;1;2(1;1),2;2;2(1;0)}, {2;2;2(1;0),1;2;2(0;0),1;1;2(0;1)}, "
      "{2;1;2(0;1),2;1;1(1;1),2;2;1(1;0)}, {2;2;1(1;0),2;2;2(0;0),2;1;2(0;1)}, "
      "{1;2;1(0;1),1;2;2(1;1),2;2;2(1;0)}, {2;2;2(1;0),2;2;1(0;0),1;2;1(0;1)}";
  EXPECT_EQ(toString(data), expected);
}

TEST_F(RendererTest, TwoFarAwayCubes) {
  // Arrange
  renderer.addBlock(0, 0, 0);
  renderer.addBlock(17 * BLOCK_W, 0, 0);

  // Act
  const auto data = drawAndGetTriangles(renderer);

  // Assert
  const std::string expected =
      "{1;0;0(0;1),0;0;0(1;1),0;1;0(1;0)}, {0;1;0(1;0),1;1;0(0;0),1;0;0(0;1)}, "
      "{0;0;1(0;1),1;0;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),0;1;1(0;0),0;0;1(0;1)}, "
      "{0;0;0(0;1),0;0;1(1;1),0;1;1(1;0)}, {0;1;1(1;0),0;1;0(0;0),0;0;0(0;1)}, "
      "{1;0;1(0;1),1;0;0(1;1),1;1;0(1;0)}, {1;1;0(1;0),1;1;1(0;0),1;0;1(0;1)}, "
      "{0;0;1(0;1),0;0;0(1;1),1;0;0(1;0)}, {1;0;0(1;0),1;0;1(0;0),0;0;1(0;1)}, "
      "{0;1;0(0;1),0;1;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),1;1;0(0;0),0;1;0(0;1)}, "

      "{137;0;0(0;1),136;0;0(1;1),136;1;0(1;0)}, {136;1;0(1;0),137;1;0(0;0),137;0;0(0;1)}, "
      "{136;0;1(0;1),137;0;1(1;1),137;1;1(1;0)}, {137;1;1(1;0),136;1;1(0;0),136;0;1(0;1)}, "
      "{136;0;0(0;1),136;0;1(1;1),136;1;1(1;0)}, {136;1;1(1;0),136;1;0(0;0),136;0;0(0;1)}, "
      "{137;0;1(0;1),137;0;0(1;1),137;1;0(1;0)}, {137;1;0(1;0),137;1;1(0;0),137;0;1(0;1)}, "
      "{136;0;1(0;1),136;0;0(1;1),137;0;0(1;0)}, {137;0;0(1;0),137;0;1(0;0),136;0;1(0;1)}, "
      "{136;1;0(0;1),136;1;1(1;1),137;1;1(1;0)}, {137;1;1(1;0),137;1;0(0;0),136;1;0(0;1)}";
  EXPECT_EQ(toString(data), expected);
}

TEST_F(RendererTest, TwoNeighborCubesOnDifferentLeaf) {
  // Arrange
  renderer.addBlock(-1, 0, 0);
  renderer.addBlock(0, 0, 0);

  // Act
  const auto data = drawAndGetTriangles(renderer);

  // Assert
  // 10 faces are expected since 2 of them are not visible
  const std::string expected =
      "{1;0;0(0;1),0;0;0(1;1),0;1;0(1;0)}, {0;1;0(1;0),1;1;0(0;0),1;0;0(0;1)}, "
      "{0;0;1(0;1),1;0;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),0;1;1(0;0),0;0;1(0;1)}, "
      "{1;0;1(0;1),1;0;0(1;1),1;1;0(1;0)}, {1;1;0(1;0),1;1;1(0;0),1;0;1(0;1)}, "
      "{0;0;1(0;1),0;0;0(1;1),1;0;0(1;0)}, {1;0;0(1;0),1;0;1(0;0),0;0;1(0;1)}, "
      "{0;1;0(0;1),0;1;1(1;1),1;1;1(1;0)}, {1;1;1(1;0),1;1;0(0;0),0;1;0(0;1)}, "

      "{0;0;0(0;1),-1;0;0(1;1),-1;1;0(1;0)}, {-1;1;0(1;0),0;1;0(0;0),0;0;0(0;1)}, "
      "{-1;0;1(0;1),0;0;1(1;1),0;1;1(1;0)}, {0;1;1(1;0),-1;1;1(0;0),-1;0;1(0;1)}, "
      "{-1;0;0(0;1),-1;0;1(1;1),-1;1;1(1;0)}, {-1;1;1(1;0),-1;1;0(0;0),-1;0;0(0;1)}, "
      "{-1;0;1(0;1),-1;0;0(1;1),0;0;0(1;0)}, {0;0;0(1;0),0;0;1(0;0),-1;0;1(0;1)}, "
      "{-1;1;0(0;1),-1;1;1(1;1),0;1;1(1;0)}, {0;1;1(1;0),0;1;0(0;0),-1;1;0(0;1)}";
  EXPECT_EQ(toString(data), expected);
}
