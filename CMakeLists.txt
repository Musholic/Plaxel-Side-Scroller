cmake_minimum_required(VERSION 3.22)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")

project(plaxel)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)

# plaxel_lib setup
set(SOURCES
        src/plaxel.cpp
        src/plaxel.h
        src/renderer/base_renderer.cpp
        src/renderer/base_renderer.h
        src/renderer/renderer.cpp
        src/renderer/renderer.h
        src/renderer/camera.cpp
        src/renderer/camera.h)

add_library(plaxel_lib STATIC ${SOURCES})

configure_file(textures/texture.jpg textures/texture.jpg COPYONLY)

target_link_libraries(plaxel_lib PRIVATE glm::glm)
target_link_libraries(plaxel_lib PRIVATE glfw)
# Public seems required for compilation of test binaries...
target_link_libraries(plaxel_lib PUBLIC Vulkan::Vulkan)
target_link_libraries(plaxel_lib PRIVATE gcov)

target_include_directories(plaxel_lib PRIVATE ${Stb_INCLUDE_DIR})

# plaxel main executable
add_executable(plaxel main.cpp)


target_link_libraries(plaxel PRIVATE plaxel_lib)
target_link_libraries(plaxel PRIVATE gcov)

include(shaderCompiling-CMakeLists.txt)
add_dependencies(plaxel_lib Shaders)

# plaxel_test setup
find_package(GTest CONFIG REQUIRED)
find_package(OpenImageIO CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread filesystem iostreams)

add_executable(plaxel_test test/plaxel.cpp
        test/renderer/renderer.cpp)

enable_testing()

configure_file(test/renderer/simple_drawing_test.ppm.gz test/renderer/simple_drawing_test.ppm.gz COPYONLY)

target_link_libraries(plaxel_test PRIVATE plaxel_lib)
target_link_libraries(plaxel_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
target_link_libraries(plaxel_test PRIVATE OpenImageIO::OpenImageIO)
target_link_libraries(plaxel_test PRIVATE Boost::boost Boost::thread Boost::filesystem Boost::iostreams)
target_link_libraries(plaxel_lib PRIVATE ZLIB::ZLIB)
target_link_libraries(plaxel_test PRIVATE gcov)

add_test(AllTestsInMain plaxel_test)

target_compile_options(plaxel PRIVATE -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic --coverage -O0 -g)
target_compile_options(plaxel_lib PRIVATE -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic --coverage -O0 -g)
target_compile_options(plaxel_test PRIVATE -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic --coverage -O0 -g)
