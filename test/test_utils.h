#ifndef TEST_UTILS_H
#define TEST_UTILS_H
#include <string>

namespace plaxel::test {

void hideWindowsByDefault();
void compressFile(const std::string &fileName, const std::string &outputFileName);
void decompressFile(const std::string &fileName, const std::string &outputFileName);
void drawAndSaveScreenshot(const char *testName);
/**
 * \return the number of comparison failures
 */
int compareImages(const char *testName);

} // namespace plaxel::test

#endif // TEST_UTILS_H
