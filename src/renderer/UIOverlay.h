#ifndef OVERLAY_H
#define OVERLAY_H

#include "camera.h"
#include "vulkan_dispatch_loader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <GLFW/glfw3.h>
#include <optional>

namespace plaxel {
// Move to another header file
struct CursorPositionBufferObject {
  glm::ivec3 pos;
};
class UIOverlay {
public:
  ~UIOverlay();
  void createDescriptorPool(const vk::raii::Device &device);
  void buildFont();

  void initialize(ImGui_ImplVulkan_InitInfo &initInfo, GLFWwindow *window, VkRenderPass renderPass,
                  const vk::raii::Device &device);
  void render(VkCommandBuffer commandBuffer);
  static std::string formatFloat(float x);
  void initNewFrame(int lastFps, Camera camera, CursorPositionBufferObject cursorPos);
  static void checkVkResult(VkResult err);
  static std::optional<std::string> testName;

private:
  vk::raii::DescriptorPool descriptorPool = nullptr;
};
} // namespace plaxel

#endif // OVERLAY_H
