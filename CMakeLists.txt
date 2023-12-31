cmake_minimum_required(VERSION 3.22)

project(plaxel)

set(CMAKE_CXX_STANDARD 20)
set(Boost_NO_WARN_NEW_VERSIONS 1)

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(CMakeRC CONFIG REQUIRED)

# plaxel_lib setup
set(SOURCES
        src/plaxel.cpp
        src/plaxel.h
        src/renderer/base_renderer.cpp
        src/renderer/base_renderer.h
        src/renderer/renderer.cpp
        src/renderer/renderer.h
        src/renderer/camera.cpp
        src/renderer/camera.h
        src/renderer/Buffer.cpp
        src/renderer/Buffer.h
        src/renderer/file_utils.cpp
        src/renderer/file_utils.h)

add_library(plaxel_lib STATIC ${SOURCES})

target_link_libraries(plaxel_lib PRIVATE glm::glm)
target_link_libraries(plaxel_lib PRIVATE glfw)
# Public seems required for compilation of test binaries...
target_link_libraries(plaxel_lib PUBLIC Vulkan::Vulkan)

target_include_directories(plaxel_lib PRIVATE ${Stb_INCLUDE_DIR})

# plaxel main executable
add_executable(plaxel main.cpp)

target_link_libraries(plaxel PRIVATE plaxel_lib)

# plaxel_test setup
find_package(GTest CONFIG REQUIRED)
find_package(OpenImageIO CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread filesystem iostreams)

add_executable(plaxel_test test/renderer/renderer.cpp)

enable_testing()

configure_file(test/renderer/simple_drawing_test.ppm.gz test/renderer/simple_drawing_test.ppm.gz COPYONLY)

target_link_libraries(plaxel_test PRIVATE plaxel_lib)
target_link_libraries(plaxel_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
target_link_libraries(plaxel_test PRIVATE OpenImageIO::OpenImageIO)
target_link_libraries(plaxel_test PRIVATE Boost::boost Boost::thread Boost::filesystem Boost::iostreams)
target_link_libraries(plaxel_lib PRIVATE ZLIB::ZLIB)

add_test(AllTestsInMain plaxel_test)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(plaxel_lib PRIVATE gcov)
    target_link_libraries(plaxel PRIVATE gcov)
    target_link_libraries(plaxel_test PRIVATE gcov)
    target_compile_options(plaxel PRIVATE -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic --coverage -O0 -g)
    target_compile_options(plaxel_lib PRIVATE -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic --coverage -O0 -g)
    target_compile_options(plaxel_test PRIVATE -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic --coverage -O0 -g)
endif ()

# Add all necessary resources embedded into the executable
include(shaderCompiling-CMakeLists.txt)

cmrc_add_resource_library(plaxel-resources ALIAS plaxel::rc NAMESPACE plaxel textures/texture.jpg)
cmrc_add_resources(plaxel-resources WHENCE ${PROJECT_BINARY_DIR} ${SPIRV_BINARY_FILES})
target_link_libraries(plaxel_lib PUBLIC plaxel-resources)

set(CPACK_PACKAGE_NAME "Plaxel")
set(CPACK_PACKAGE_VENDOR "Tiny Blocks Games")

if (WIN32)
    set(CPACK_GENERATOR "ZIP")
    # Prevent console window from showing up
    target_link_options(plaxel PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    install(FILES ${PROJECT_BINARY_DIR}/Release/glfw3.dll DESTINATION .)
else ()
    set(CPACK_GENERATOR "TGZ")
endif ()

install(TARGETS plaxel RUNTIME DESTINATION .)
include(CPack)


