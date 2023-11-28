#include "file_utils.h"
#include "cmrc/cmrc.hpp"

CMRC_DECLARE(plaxel);

namespace plaxel::files {

cmrc::file readFile(const std::string &filename) {
  auto fs = cmrc::plaxel::get_filesystem();
  return fs.open(filename);
}
} // namespace plaxel::files