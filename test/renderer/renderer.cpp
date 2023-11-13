#include "../../src/renderer/renderer.h"
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <filesystem>
#include <gtest/gtest.h>

using namespace plaxel;

void hideWindowsByDefault() {
  glfwInit();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
}

TEST(RendererTest, Test) {
  // Arrange
  hideWindowsByDefault();

  Renderer r;
  r.showWindow();

  // Act
  // We need to draw 2 frames, to free up one of the swapChainImage so we can use it to save our
  // screenshot
  r.draw();
  r.draw();

  // Assert
  std::filesystem::create_directory("test_report");
  r.saveScreenshot("test_report/test_result.ppm");
  r.closeWindow();

  OIIO::ImageBuf refTestImage("test/renderer/simple_drawing_test.ppm");
  OIIO::ImageBuf testResultImage("test_report/test_result.ppm");

  const OIIO::ImageSpec &spec = testResultImage.spec();
  int xres = spec.width;
  int yres = spec.height;
  std::cout << "Spec: " << xres << "x" << yres << std::endl;

  auto comp = OIIO::ImageBufAlgo::compare(testResultImage, refTestImage, 1.0f / 255.0f, 0.0f);

  if (comp.nwarn == 0 && comp.nfail == 0) {
    std::cout << "Images match within tolerance\n";
  } else {
    std::cout << "Image differed: " << comp.nfail << " failures, " << comp.nwarn << " warnings.\n";
    std::cout << "Average error was " << comp.meanerror << "\n";
    std::cout << "RMS error was " << comp.rms_error << "\n";
    std::cout << "PSNR was " << comp.PSNR << "\n";
    std::cout << "largest error was " << comp.maxerror << " on pixel (" << comp.maxx << ","
              << comp.maxy << "," << comp.maxz << "), channel " << comp.maxc << "\n";
  }

  const OpenImageIO_v2_4::ImageBuf &diff =
      OIIO::ImageBufAlgo::absdiff(refTestImage, testResultImage);
  diff.write("test_report/diff.ppm");

  EXPECT_EQ(comp.nfail, 0);
}
