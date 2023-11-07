cmake_minimum_required(VERSION 3.27)
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
        src/renderer/renderer.h)

add_library(plaxel_lib STATIC ${SOURCES})

configure_file(textures/texture.jpg textures/texture.jpg COPYONLY)

target_link_libraries(plaxel_lib PRIVATE glm::glm)
target_link_libraries(plaxel_lib PRIVATE glfw)
target_link_libraries(plaxel_lib PRIVATE Vulkan::Vulkan)

target_include_directories(plaxel_lib PRIVATE ${Stb_INCLUDE_DIR})

# plaxel main executable
add_executable(plaxel main.cpp)

target_link_libraries(plaxel PRIVATE plaxel_lib)

include(shaderCompiling-CMakeLists.txt)

# plaxel_test setup
find_package(GTest CONFIG REQUIRED)

add_executable(plaxel_test test/plaxel.cpp)

enable_testing()

target_link_libraries(plaxel_test PRIVATE plaxel_lib)
target_link_libraries(plaxel_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

add_test(AllTestsInMain plaxel_test)
