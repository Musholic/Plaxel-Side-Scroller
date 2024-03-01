#include "UIOverlay.h"

#include "file_utils.h"
#include "version.h"

#include <array>
#include <cstdio>
#include <cstdlib>

using namespace plaxel;
std::optional<std::string> UIOverlay::testName;

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

void UIOverlay::buildFont() {
  const ImGuiIO &io = ImGui::GetIO();
  const auto fontFile = files::readFile("fonts/NotoSans-Regular.ttf");
  ImFontConfig fontConfig{};
  fontConfig.FontData = std::bit_cast<void *>(fontFile.begin());
  fontConfig.FontDataSize = static_cast<int>(fontFile.size());
  fontConfig.SizePixels = 32;
  // The font data is automatically freed by us
  fontConfig.FontDataOwnedByAtlas = false;
  io.Fonts->AddFont(&fontConfig);
  io.Fonts->Build();
}

void UIOverlay::initialize(ImGui_ImplVulkan_InitInfo &initInfo, GLFWwindow *window,
                           const VkRenderPass renderPass, const vk::raii::Device &device) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  buildFont();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  ImGui_ImplGlfw_InitForVulkan(window, true);

  initInfo.CheckVkResultFn = checkVkResult;

  createDescriptorPool(device);

  initInfo.Device = *device;
  initInfo.DescriptorPool = *descriptorPool;

  ImGui_ImplVulkan_Init(&initInfo, renderPass);
}

void UIOverlay::initNewFrame(const int lastFps, Camera camera,
                             CursorPositionBufferObject cursorPos) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  ImGui::SetNextWindowPos(ImVec2(0, 0));

  ImGui::SetNextWindowSize(ImVec2(800, 300));
  ImGui::Begin("Debug info", nullptr, windowFlags);
  std::string title;
  if (testName.has_value()) {
    title += "TDD: ";
    title += *testName;
  } else {
    title += "PlaxelSS v";
    title += PROJECT_GIT_VERSION;
  }
  ImGui::Text(title.c_str());
  std::string fpsText = "FPS: ";
  fpsText += std::to_string(lastFps);
  ImGui::Text(fpsText.c_str());
  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 100));
  ImGui::SetNextWindowSize(ImVec2(800, 100));
  ImGui::Begin("Debug info (down)", nullptr, windowFlags);
  std::string cameraText = "Camera: ";
  cameraText += formatFloat(camera.position.x) + ";";
  cameraText += formatFloat(camera.position.y) + ";";
  cameraText += formatFloat(camera.position.z) + "{";
  cameraText += formatFloat(camera.rotation.x) + ";";
  cameraText += formatFloat(camera.rotation.y) + ";";
  cameraText += formatFloat(camera.rotation.z) + "}";
  ImGui::Text(cameraText.c_str());
  std::string cursorText = "Cursor: ";
  cursorText += std::to_string(cursorPos.pos.x) + ";";
  cursorText += std::to_string(cursorPos.pos.y) + ";";
  cursorText += std::to_string(cursorPos.pos.z);
  ImGui::Text(cursorText.c_str());
  ImGui::End();
}

void UIOverlay::render(VkCommandBuffer commandBuffer) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

std::string UIOverlay::formatFloat(float x) {
  char result[20];
  std::sprintf(result, "%.1f", x);
  return result;
}

void UIOverlay::checkVkResult(VkResult err) {
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}
