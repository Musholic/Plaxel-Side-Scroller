#include "../../src/renderer/renderer.h"
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <filesystem>
#include <gtest/gtest.h>

using namespace plaxel;

void hideWindowsByDefault() {
  glfwInit();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
}

void compressFile(const std::string &fileName, const std::string &outputFileName) {
  std::ifstream file(fileName, std::ios_base::in | std::ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  in.push(boost::iostreams::gzip_compressor());
  in.push(file);

  std::ofstream fileOut(outputFileName, std::ios_base::out | std::ios_base::binary);
  boost::iostreams::copy(in, fileOut);
  fileOut.close();
}

void decompressFile(const std::string &fileName, const std::string &outputFileName) {
  std::ifstream file(fileName, std::ios_base::in | std::ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  in.push(boost::iostreams::gzip_decompressor());
  in.push(file);

  std::ofstream fileOut(outputFileName, std::ios_base::out | std::ios_base::binary);
  boost::iostreams::copy(in, fileOut);
  fileOut.close();
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
  std::filesystem::create_directory("workTmp");
  r.saveScreenshot("workTmp/test_result.ppm");
  r.closeWindow();

  compressFile("workTmp/test_result.ppm", "test_report/test_result.ppm.gz");
  decompressFile("test/renderer/simple_drawing_test.ppm.gz", "workTmp/expected.ppm");

  const OIIO::ImageBuf refTestImage("workTmp/expected.ppm");
  const OIIO::ImageBuf testResultImage("workTmp/test_result.ppm");

  const OIIO::ImageSpec &spec = testResultImage.spec();
  const int xres = spec.width;
  const int yres = spec.height;
  std::cout << "Spec: " << xres << "x" << yres << std::endl;

  const auto comp = OIIO::ImageBufAlgo::compare(testResultImage, refTestImage, 3.0f / 255.0f, 0.0f);

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
  diff.write("workTmp/diff.ppm");
  compressFile("workTmp/diff.ppm", "test_report/diff.ppm.gz");

  EXPECT_EQ(comp.nfail, 0);
}
