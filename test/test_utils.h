#ifndef TEST_UTILS_H
#define TEST_UTILS_H
#include "renderer/TestRenderer.h"

#include <gtest/gtest.h>
#include <string>

namespace plaxel::test {

const bool SHOW_WINDOW = std::getenv("SHOW_WINDOW");

void hideWindowsByDefault();
void compressFile(const std::string &fileName, const std::string &outputFileName);
void decompressFile(const std::string &fileName, const std::string &outputFileName);
void drawAndSaveScreenshot(TestRenderer &, const char *testName);
[[nodiscard]] std::vector<Triangle> drawAndGetTriangles(TestRenderer &renderer);
/**
 * \return the number of comparison failures
 */
int compareImages(const char *testName);
[[nodiscard]] std::string toString(std::vector<Triangle> const &vec);

class RendererTest : public ::testing::Test {
protected:
  TestRenderer renderer;
  void SetUp() override {
    UIOverlay::testName =
        std::string(::testing::UnitTest::GetInstance()->current_test_info()->name());
    if (!SHOW_WINDOW) {
      hideWindowsByDefault();
      renderer.showOverlay = false;
    }
    renderer.showWindow();
  }
  void TearDown() override {
    if (SHOW_WINDOW) {
      while (!renderer.shouldClose()) {
        renderer.draw();
      }
    }
    renderer.closeWindow();
  }
};

class BenchmarkRendererTest : public RendererTest {
public:
  double benchmark();

protected:
  void SetUp() override {
    RendererTest::SetUp();

    if (!SHOW_WINDOW) {
      setenv("DISABLE_FPS_LIMIT", "1", 0);
    }
  }
};
} // namespace plaxel::test

#endif // TEST_UTILS_H
