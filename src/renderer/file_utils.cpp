#include "file_utils.h"

CMRC_DECLARE(plaxel);

namespace plaxel::files {

cmrc::file readFile(const std::string &filename) {
  const auto fs = cmrc::plaxel::get_filesystem();
  return fs.open(filename);
}
} // namespace plaxel::files