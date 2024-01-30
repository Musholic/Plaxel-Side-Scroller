#include "UIOverlay.h"

#include <array>
#include <cstdio>
#include <cstdlib>

using namespace plaxel;

UIOverlay::~UIOverlay() {
  if (ImGui::GetCurrentContext()) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }
}

void UIOverlay::createDescriptorPool(const vk::raii::Device &device) {
  std::array<vk::DescriptorPoolSize, 1> poolSizes;
  poolSizes[0].type = vk::DescriptorType::eCombinedImageSampler;
  poolSizes[0].descriptorCount = 1;

  vk::DescriptorPoolCreateInfo poolInfo;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 1;
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

  descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void UIOverlay::initialize(ImGui_ImplVulkan_InitInfo &initInfo, GLFWwindow *window,
                           VkRenderPass renderPass, const vk::raii::Device &device) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontDefault();
  io.Fonts->Build();

  ImGui_ImplGlfw_InitForVulkan(window, true);

  initInfo.CheckVkResultFn = checkVkResult;

  createDescriptorPool(device);

  initInfo.Device = *device;
  initInfo.DescriptorPool = *descriptorPool;

  ImGui_ImplVulkan_Init(&initInfo, renderPass);
}

void UIOverlay::initNewFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();
}

void UIOverlay::render(VkCommandBuffer commandBuffer) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void UIOverlay::checkVkResult(VkResult err) {
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}
