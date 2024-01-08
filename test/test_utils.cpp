#include "test_utils.h"
#include "../src/renderer/renderer.h"
#include "renderer/TestRenderer.h"

#include <GLFW/glfw3.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <filesystem>
#include <fstream>
#include <numeric>

using namespace plaxel::test;

void plaxel::test::hideWindowsByDefault() {
  glfwInit();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
}

void plaxel::test::compressFile(const std::string &fileName, const std::string &outputFileName) {
  std::ifstream file(fileName, std::ios_base::in | std::ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  in.push(boost::iostreams::gzip_compressor());
  in.push(file);

  std::ofstream fileOut(outputFileName, std::ios_base::out | std::ios_base::binary);
  boost::iostreams::copy(in, fileOut);
  fileOut.close();
}

void plaxel::test::decompressFile(const std::string &fileName, const std::string &outputFileName) {
  std::ifstream file(fileName, std::ios_base::in | std::ios_base::binary);
  boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
  in.push(boost::iostreams::gzip_decompressor());
  in.push(file);

  std::ofstream fileOut(outputFileName, std::ios_base::out | std::ios_base::binary);
  boost::iostreams::copy(in, fileOut);
  fileOut.close();
}

void plaxel::test::drawAndSaveScreenshot(const char *testName) {
  hideWindowsByDefault();

  Renderer r;
  r.showWindow();

  // We need to draw 2 frames, to free up one of the swapChainImage so we can use it to save our
  // screenshot
  r.draw();
  r.draw();

  std::filesystem::create_directory("workTmp");
  std::string fileName = "workTmp/";
  fileName += testName;
  fileName += ".ppm";
  r.saveScreenshot(fileName.c_str());
  r.closeWindow();
}

std::vector<Triangle> plaxel::test::drawAndGetTriangles(TestRenderer &renderer) {
  if (!SHOW_WINDOW) {
    hideWindowsByDefault();
  }
  renderer.showWindow();

  // We need to draw 2 frames, to free up one of the swapChainImage so we can use it to save our
  // screenshot
  renderer.draw();
  auto indexBufferData = renderer.getTriangles();

  if (!SHOW_WINDOW) {
    renderer.closeWindow();
  }

  return indexBufferData;
}

int plaxel::test::compareImages(const char *testName) {
  std::filesystem::create_directory("test_report");

  const std::string testResultFileName = std::string("workTmp/").append(testName).append(".ppm");

  const std::string testResultReportFileName =
      std::string("test_report/").append(testName).append(".ppm.gz");

  compressFile(testResultFileName, testResultReportFileName);
  const auto refTestFileNameCompressed =
      std::string("test/renderer/").append(testName).append(".ppm.gz");
  const auto refTestFileName = std::string("workTmp/").append(testName).append("_expected.ppm");
  decompressFile(refTestFileNameCompressed, refTestFileName);

  const OIIO::ImageBuf refTestImage(refTestFileName);
  const OIIO::ImageBuf testResultImage(testResultFileName);

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

    const OpenImageIO_v2_4::ImageBuf &diff =
        OIIO::ImageBufAlgo::absdiff(refTestImage, testResultImage);

    const auto testResultDiffFileName =
        std::string("workTmp/").append(testName).append("_diff.ppm");
    diff.write(testResultDiffFileName);

    const auto testResultDiffFileNameCompressed =
        std::string("test_report/").append(testName).append("_diff.ppm.gz");
    compressFile(testResultDiffFileName, testResultDiffFileNameCompressed);
  }

  return static_cast<int>(comp.nfail);
}

std::string plaxel::test::toString(std::vector<Triangle> const &vec) {
  if (vec.empty()) {
    return {};
  }

  return std::accumulate(
      vec.begin() + 1, vec.end(), vec[0].toString(),
      [](const std::string &a, const Triangle &b) { return a + ", " + b.toString(); });
}