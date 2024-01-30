#ifndef OVERLAY_H
#define OVERLAY_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

namespace plaxel {
class UIOverlay {
public:
  ~UIOverlay();
  void createDescriptorPool(const vk::raii::Device &device);

  void initialize(ImGui_ImplVulkan_InitInfo &initInfo, GLFWwindow *window, VkRenderPass renderPass,
                  const vk::raii::Device &device);
  void render(VkCommandBuffer commandBuffer);
  void initNewFrame();
  static void checkVkResult(VkResult err);

private:
  vk::raii::DescriptorPool descriptorPool = nullptr;
};
} // namespace plaxel

#endif // OVERLAY_H
