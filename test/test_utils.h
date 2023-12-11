#ifndef TEST_UTILS_H
#define TEST_UTILS_H
#include "renderer/TestRenderer.h"

#include <string>

namespace plaxel::test {

void hideWindowsByDefault();
void compressFile(const std::string &fileName, const std::string &outputFileName);
void decompressFile(const std::string &fileName, const std::string &outputFileName);
void drawAndSaveScreenshot(const char *testName);
[[nodiscard]] std::vector<Triangle> drawAndGetTriangles();
/**
 * \return the number of comparison failures
 */
int compareImages(const char *testName);
[[nodiscard]] std::string toString(std::vector<Triangle> const &vec);

} // namespace plaxel::test

#endif // TEST_UTILS_H
