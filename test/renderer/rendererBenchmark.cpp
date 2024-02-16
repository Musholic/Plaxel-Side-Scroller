#include "../test_utils.h"

using namespace plaxel;
using namespace plaxel::test;
constexpr int MAX_BENCHMARK_TIME_MS = 1000;

TEST_F(BenchmarkRendererTest, BigCircleBenchmark) {
  // Arrange
  setenv("DISABLE_FPS_LIMIT", "1", 0);
  // TODO: check why there are some missing faces, may be fixed with the mesh shader
  constexpr int circleSize = 20;
  for (int x = -circleSize; x <= circleSize; x++) {
    for (int y = -circleSize; y <= circleSize; y++) {
      if (x * x + y * y <= circleSize * circleSize) {
        for (int z = 0; z < BLOCK_W; z++) {
          renderer.addBlock(x, y, z);
        }
      }
    }
  }

  // Act
  double average_ms = benchmark();

  // Assert
  EXPECT_LT(average_ms, 30);
}
