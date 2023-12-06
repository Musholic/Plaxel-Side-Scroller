#ifndef PLAXEL_FILE_UTILS_H
#define PLAXEL_FILE_UTILS_H

#include <cmrc/cmrc.hpp>
#include <string>

namespace plaxel::files {

cmrc::file readFile(const std::string &filename);

}

#endif