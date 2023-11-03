cmake_minimum_required(VERSION 3.27)
project(plaxel)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(plaxel main.cpp
        src/renderer/base_renderer.cpp
        src/renderer/base_renderer.h
        src/plaxel.cpp
        src/plaxel.h
)

target_link_libraries(plaxel PRIVATE glm::glm)
target_link_libraries(plaxel PRIVATE glfw)
target_link_libraries(plaxel PRIVATE Vulkan::Vulkan)

include(shaderCompiling-CMakeLists.txt)